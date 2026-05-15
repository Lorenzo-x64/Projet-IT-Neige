#!/usr/bin/env python3
"""
Snow Transceiver -- offline map tile downloader
================================================

Downloads OpenStreetMap slippy-map tiles for a chosen region into the
directory layout the firmware expects on the SD card:

    /tiles/{z}/{x}/{y}.png

Once downloaded, copy the entire `tiles/` directory to the **root of the
SD card** that goes into the device.  The web UI's GPS map will then show
real raster tiles instead of the vector graticule fallback.

USAGE
-----
    # Tiles for a 5 km box around your home, zoom 14..16:
    python3 download_tiles.py \\
        --lat 46.4628 --lon 8.4137 \\
        --radius-km 5 \\
        --zoom-min 14 --zoom-max 16 \\
        --out ./tiles

    # Tiles for an explicit bounding box (south, west, north, east):
    python3 download_tiles.py \\
        --bbox 46.40 8.30 46.55 8.55 \\
        --zoom-min 13 --zoom-max 17 \\
        --out ./tiles

LICENSING / FAIR USE
--------------------
OpenStreetMap tiles are donated by volunteers.  Their tile-usage policy
asks bulk downloaders to:
  - identify themselves with a real User-Agent
  - not exceed ~2 requests/second from a single client
  - cache aggressively to avoid re-downloading
This script does all three by default.  Be reasonable: a million-tile
download is abuse, a few-thousand-tile download for personal use is fine.
For larger areas, use a self-hosted tile server or commercial provider.

Reference: https://operations.osmfoundation.org/policies/tiles/
"""

import argparse
import math
import os
import sys
import time
import urllib.request
import urllib.error

# ---------------------------------------------------------------------------
DEFAULT_TILE_URL = "https://tile.openstreetmap.org/{z}/{x}/{y}.png"
USER_AGENT       = "SnowTransceiver/1.0 (offline-map; +https://example.invalid)"
RATE_LIMIT_S     = 0.55     # ~2 req/s, slightly under

# ---------------------------------------------------------------------------
# Slippy-map tile math (Web Mercator)
# ---------------------------------------------------------------------------
def lonlat_to_tile(lon, lat, zoom):
    """Convert WGS84 lon/lat to (x_tile, y_tile) at the given zoom level."""
    n = 1 << zoom
    x = int((lon + 180.0) / 360.0 * n)
    lat_r = math.radians(lat)
    y = int((1.0 - math.log(math.tan(lat_r) + 1.0 / math.cos(lat_r)) / math.pi) / 2.0 * n)
    return x, y

def km_to_deg(km, lat):
    """Approximate km -> degrees, separately for lat and lon at given latitude."""
    dlat = km / 111.0
    dlon = km / (111.0 * max(0.01, math.cos(math.radians(lat))))
    return dlat, dlon

# ---------------------------------------------------------------------------
def parse_args():
    ap = argparse.ArgumentParser(
        description="Download OSM tiles for offline use by the Snow Transceiver firmware",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--bbox", nargs=4, type=float,
                   metavar=("S", "W", "N", "E"),
                   help="Bounding box: south west north east (decimal degrees)")
    g.add_argument("--lat", type=float,
                   help="Centre latitude (used with --lon and --radius-km)")
    ap.add_argument("--lon", type=float,
                    help="Centre longitude")
    ap.add_argument("--radius-km", type=float, default=5.0,
                    help="Half-side of the box around the centre point (default: 5 km)")

    ap.add_argument("--zoom-min", type=int, default=14, help="Lowest zoom level (default 14)")
    ap.add_argument("--zoom-max", type=int, default=16, help="Highest zoom level (default 16)")
    ap.add_argument("--out", default="./tiles", help="Output directory (default ./tiles)")

    ap.add_argument("--url", default=DEFAULT_TILE_URL,
                    help="Tile URL template with {z}/{x}/{y} placeholders")
    ap.add_argument("--user-agent", default=USER_AGENT,
                    help="Override the User-Agent header")
    ap.add_argument("--max-tiles", type=int, default=20000,
                    help="Hard cap to prevent runaway downloads (default 20000)")
    ap.add_argument("--dry-run", action="store_true",
                    help="Print plan and tile counts but don't download")

    args = ap.parse_args()

    if args.lat is not None and args.lon is None:
        ap.error("--lat requires --lon")
    if args.zoom_min < 0 or args.zoom_max > 19 or args.zoom_min > args.zoom_max:
        ap.error("zoom range must be 0..19 with min <= max")
    return args

# ---------------------------------------------------------------------------
def planning(args):
    if args.bbox:
        s, w, n, e = args.bbox
    else:
        dlat, dlon = km_to_deg(args.radius_km, args.lat)
        s, n = args.lat - dlat, args.lat + dlat
        w, e = args.lon - dlon, args.lon + dlon
    print(f"  Bounding box: S={s:.5f}  W={w:.5f}  N={n:.5f}  E={e:.5f}")

    plan = []   # list of (z, x, y)
    for z in range(args.zoom_min, args.zoom_max + 1):
        x_min, y_min = lonlat_to_tile(w, n, z)
        x_max, y_max = lonlat_to_tile(e, s, z)
        x_lo, x_hi = sorted((x_min, x_max))
        y_lo, y_hi = sorted((y_min, y_max))
        count = (x_hi - x_lo + 1) * (y_hi - y_lo + 1)
        print(f"  z={z:>2}  x={x_lo}..{x_hi}  y={y_lo}..{y_hi}  ({count} tiles)")
        for x in range(x_lo, x_hi + 1):
            for y in range(y_lo, y_hi + 1):
                plan.append((z, x, y))
    return plan

# ---------------------------------------------------------------------------
def fetch_one(url, ua, dest):
    req = urllib.request.Request(url, headers={"User-Agent": ua})
    try:
        with urllib.request.urlopen(req, timeout=20) as r:
            data = r.read()
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "wb") as f:
            f.write(data)
        return True, len(data)
    except urllib.error.HTTPError as e:
        return False, f"HTTP {e.code}"
    except Exception as e:
        return False, str(e)

def main():
    args = parse_args()
    print("=" * 70)
    print("  Snow Transceiver -- offline tile downloader")
    print("=" * 70)
    plan = planning(args)
    total = len(plan)
    print(f"\n  Total tiles to fetch: {total}")
    print(f"  Output directory:     {os.path.abspath(args.out)}")
    print(f"  Rate limit:           ~{1.0/RATE_LIMIT_S:.1f} req/s")
    print(f"  Estimated time:       ~{total * RATE_LIMIT_S / 60:.1f} min\n")

    if total > args.max_tiles:
        print(f"  ABORT: {total} > --max-tiles ({args.max_tiles}). Reduce zoom range or area.")
        sys.exit(2)

    if args.dry_run:
        print("  --dry-run set, exiting without downloading.")
        return

    ok = skipped = failed = 0
    bytes_total = 0
    t0 = time.time()
    for i, (z, x, y) in enumerate(plan, 1):
        dest = os.path.join(args.out, str(z), str(x), f"{y}.png")
        if os.path.exists(dest) and os.path.getsize(dest) > 0:
            skipped += 1
            continue
        url = args.url.format(z=z, x=x, y=y)
        success, info = fetch_one(url, args.user_agent, dest)
        if success:
            ok += 1
            bytes_total += info
        else:
            failed += 1
            print(f"  ! z={z} x={x} y={y}: {info}")
        if i % 25 == 0 or i == total:
            elapsed = time.time() - t0
            rate = i / max(0.1, elapsed)
            eta = (total - i) / max(0.1, rate)
            print(f"  [{i:>5}/{total}]  ok={ok}  skip={skipped}  fail={failed}  "
                  f"{bytes_total/1024:.0f} KB  {rate:.1f}/s  ETA {eta/60:.1f}m")
        time.sleep(RATE_LIMIT_S)

    print("\n" + "=" * 70)
    print(f"  Done. ok={ok}  skipped={skipped}  failed={failed}  "
          f"size={bytes_total/(1024*1024):.1f} MB")
    print(f"  Copy the contents of '{args.out}' to your SD card root so the")
    print(f"  firmware sees a /tiles/{{z}}/{{x}}/{{y}}.png tree.")
    print("=" * 70)

if __name__ == "__main__":
    main()
