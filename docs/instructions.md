# Instructions

## Creating a virtual MIDI channel

1. Download [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)
2. Make a port channel by typing in `FLRP` in the `New port-name:` textbox, and then hit the plus sign

## Adding the script into FL Studio

1. Go into your FL Studio files which can typically be found `C:\Users\<USER NAME>\Documents\Image-Line\FL Studio\Settings\Hardware\`
2. Create Folder called FLRP
3. Place `device_FLRP` inside

## Initializing

1. Go into your FL Studio MIDI Settingss
2. In the output section, **disable** `Send Master Sync`
3. In the input section, for controller type, select `FLRP Script`
4. Set both ports to `0`

### Note: do not use discord_client.py, while it does work, this was purely used as a reference file
