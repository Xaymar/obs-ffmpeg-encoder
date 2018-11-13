# FFMPEG Encoders for OBS studio
Adds ffmpeg encoders for normal use instead of requiring ffmpeg output. Not much else to say.

# Encoders
## Video Production / Edition
### Apple ProRes (prores_aw)
* Intra-Frame only
* Fixed Bitrate (defined by Profile)
* Profiles: PXY, LT, Standard, HQ, 4444
* Threading: Frame

# Threading Modes
All threading modes can be combined to create a more complex one.
## Frame
Frame Threading means that new frames can be submitted before the last one is complete. This is the most basic threading and has huge latency issues, and is likely also very unstable with frame times.

## Slice
Slice Threading allows individual frames to be done quicker by splitting up the entire submitted frame into individual slices, each of which are handed off to workers. More complex than frame threading, but can result in faster encoding and will result in less latency.

## Macroblock
This kind of threading is rarely seen and splits the frame into macroblocks, allowing for massive parallelization. Similar latency to Slice threading and can occasionally use SMT/HT better, resulting in better performance.
