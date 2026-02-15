"""
O-Texture Rain Dataset Download
Downloads rain audio from Freesound.org via API.

Prerequisites:
  1. pip install freesound-python
  2. Get API key from https://freesound.org/apiv2/apply/
  3. Set FREESOUND_API_KEY environment variable

Usage:
  python download_rain.py --api-key YOUR_KEY
  # or
  FREESOUND_API_KEY=YOUR_KEY python download_rain.py
"""

import os
import argparse
from pathlib import Path

import config

# Curated high-quality rain sound IDs from Freesound.org
# Each entry: (sound_id, description, approx_duration_seconds)
CURATED_RAIN_SOUNDS = [
    # Light drizzle / gentle rain
    (11485,  "inchadney - light rain on leaves", 180),
    (39828,  "Arctura - rain ambient loop", 60),
    (320520, "Opticreep - light rain field recording", 120),

    # Steady medium rain
    (243627, "BonnyOrbit - steady rain recording", 300),
    (244495, "BonnyOrbit - moderate rain", 240),
    (335725, "InspectorJ - rain medium", 120),

    # Heavy rain / downpour
    (256483, "RJStefanski - heavy thunderstorm rain", 600),
    (346642, "be-steele - heavy rain storm", 300),

    # Rain on surfaces
    (401275, "BonnyOrbit - rain on roof", 180),
    (216679, "klankbeeld - rain on metal", 120),

    # Rain with atmosphere
    (346556, "be-steele - rain with distant thunder", 240),
    (243628, "BonnyOrbit - rain forest atmosphere", 300),
]

# Search queries for additional sounds
SEARCH_QUERIES = [
    "rain field recording",
    "rain ambient",
    "heavy rain",
    "light rain drizzle",
    "rain on roof",
    "rainfall",
]


def download_curated(api_key, output_dir):
    """Download curated list of rain sounds."""
    try:
        import freesound
    except ImportError:
        print("ERROR: freesound-python not installed.")
        print("Run: pip install freesound-python")
        return

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    client = freesound.FreesoundClient()
    client.set_token(api_key, "token")

    downloaded = 0
    for sound_id, description, _ in CURATED_RAIN_SOUNDS:
        out_path = output_dir / f"rain_{sound_id}.wav"
        if out_path.exists():
            print(f"  SKIP {sound_id} (already downloaded)")
            downloaded += 1
            continue

        try:
            sound = client.get_sound(sound_id)
            sound.retrieve(str(output_dir), name=f"rain_{sound_id}")
            print(f"  OK {sound_id}: {description}")
            downloaded += 1
        except Exception as e:
            print(f"  FAIL {sound_id}: {e}")

    print(f"\nDownloaded {downloaded}/{len(CURATED_RAIN_SOUNDS)} curated sounds")


def search_and_download(api_key, output_dir, max_per_query=5):
    """Search Freesound for additional rain sounds."""
    try:
        import freesound
    except ImportError:
        print("ERROR: freesound-python not installed.")
        return

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    client = freesound.FreesoundClient()
    client.set_token(api_key, "token")

    total = 0
    for query in SEARCH_QUERIES:
        print(f"\nSearching: '{query}'")
        try:
            results = client.text_search(
                query=query,
                filter="tag:rain duration:[30 TO 600]",
                sort="rating_desc",
                fields="id,name,duration,samplerate,license",
                page_size=max_per_query
            )
            for sound in results:
                out_path = output_dir / f"rain_{sound.id}.wav"
                if out_path.exists():
                    continue
                try:
                    sound.retrieve(str(output_dir), name=f"rain_{sound.id}")
                    print(f"  OK {sound.id}: {sound.name} ({sound.duration:.0f}s)")
                    total += 1
                except Exception as e:
                    print(f"  FAIL {sound.id}: {e}")
        except Exception as e:
            print(f"  Search failed: {e}")

    print(f"\nDownloaded {total} additional sounds from search")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Download rain audio from Freesound")
    parser.add_argument("--api-key", type=str,
                        default=os.environ.get("FREESOUND_API_KEY"),
                        help="Freesound API key")
    parser.add_argument("--output", type=str, default=None,
                        help="Output directory (default: training/data/raw/)")
    parser.add_argument("--search", action="store_true",
                        help="Also search for additional sounds beyond curated list")
    args = parser.parse_args()

    if not args.api_key:
        print("ERROR: No API key provided.")
        print("Set FREESOUND_API_KEY env var or use --api-key")
        print("Get a key at: https://freesound.org/apiv2/apply/")
        exit(1)

    output_dir = args.output or str(config.DATA_RAW_DIR)
    print(f"Output directory: {output_dir}")
    print()

    print("=== Downloading curated rain sounds ===")
    download_curated(args.api_key, output_dir)

    if args.search:
        print("\n=== Searching for additional rain sounds ===")
        search_and_download(args.api_key, output_dir)

    print("\nDone! Next step: python preprocess.py")
