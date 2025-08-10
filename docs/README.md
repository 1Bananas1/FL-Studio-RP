<div  align="center">

# FL Studio Rich Presence for Discord

</div>
<br>

## Key Features

- **Low Memory Usage**: All monitoring and discord interactions done with C++
- **Active State Updating**: Shows your current activity whether you are listening, recording, composing, and more!
  <br>

## Instructions

### Creating a virtual MIDI channel

1. Download [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html)
2. Make a port channel by typing in `FLRP` in the `New port-name:` textbox, and then hit the plus sign

### Initializing

1. Go into your FL Studio MIDI Settings
2. In the output section, **disable** `Send Master Sync`
3. In the input section, for controller type, select `FLRP Script`
4. Set both ports to `0`

<br>

## FAQ

**Q: My FL Studio isn't updating!**

> A: There are two potential reasons, there is a known bug where if you open a new project, the script will stop working. To fix this, go to View -> Script Output, and then hit the reload script button.

**Q: Why isn't my discord updating every second?**

> A: Discord rate limits how often you can update this information, it is updating as fast as it can!

**Q: My FL Studio experiences extreme lag after running the script!**

> A: This is an easy fix! Go to your MIDI settings and disable `Send Master Sync`

**Q: Can I use this while using an actual MIDI keyboard?**

> A: While I haven't tested this, if you can get me the python script that your controller uses, I **might** be able to work something out! You can find the folders here: `C:\Users\<USER NAME>\Documents\Image-Line\FL Studio\Settings\Hardware`.

**Q: I'm getting a state file not found error**

> A: In order to initialize, you first need to run the python script inside of FL Studio, then you can launch FLRP!

**Q: It's saying that my .env file cannot be found?**

> A: Try starting the exe from the "C:\Program Files (x86)\FL Studio Rich Presence" folder!