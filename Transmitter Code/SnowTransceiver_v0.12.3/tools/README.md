# Tools

Small utilities that complement the firmware. Run with Python 3.8+.

## download_tiles.py

Fetches OpenStreetMap raster tiles for a region and lays them out in the
directory tree the firmware reads from the SD card:

```
tiles/
├── 14/
│   ├── 8573/
│   │   ├── 5797.png
│   │   ├── 5798.png
│   │   └── ...
│   └── 8574/
│       └── ...
├── 15/
└── 16/
```

After running the script, copy `tiles/` to the **root of the SD card** so
the firmware sees `/tiles/{z}/{x}/{y}.png`.

### Quick examples

```bash
# 5 km square around Andermatt, zoom 14..16 (~few hundred tiles)
python3 download_tiles.py \
    --lat 46.6376 --lon 8.5942 \
    --radius-km 5 \
    --zoom-min 14 --zoom-max 16 \
    --out ./tiles

# Explicit bounding box
python3 download_tiles.py \
    --bbox 46.40 8.30 46.55 8.55 \
    --zoom-min 13 --zoom-max 17 \
    --out ./tiles

# See what would happen without downloading anything
python3 download_tiles.py --lat 46.5 --lon 8.4 --radius-km 10 \
                          --zoom-min 12 --zoom-max 18 --dry-run
```

### Picking a zoom range

| Zoom | Tile size at 46° N | Use case                                  |
|------|--------------------|-------------------------------------------|
| 12   | ~5 km              | Country-level overview                    |
| 14   | ~1.2 km            | City / small region                       |
| 16   | ~300 m             | Neighbourhood, station deployment context |
| 18   | ~75 m              | Building footprint, fine detail           |

For a typical mountain-station deployment, `14..16` is a sensible default
(a 5 km radius, 3 zoom levels = a few hundred tiles, ~5–10 MB on SD).

### Be a good citizen

The default tile server is `tile.openstreetmap.org`. OSM's volunteer
infrastructure is donated by the community. The script is rate-limited to
~2 req/s, sets a real `User-Agent`, and skips already-downloaded tiles, in
line with [OSM's tile-usage policy](https://operations.osmfoundation.org/policies/tiles/).
For very large downloads or commercial use, point `--url` at a different
provider (Mapbox, MapTiler, your own self-hosted server, etc.).

### Tile format expected by firmware

* Standard slippy-map (XYZ) layout, Web Mercator
* PNG, 256×256 px (the firmware doesn't enforce size, but the JS map
  assumes 256-px tiles)
* Path: `/tiles/{z}/{x}/{y}.png` on the SD card
