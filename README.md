# Microphone-Router
Route your microphone data over the intranet or internet.

## Motivation
I wanted to use my Phone's microphone as my laptop's, so here I am. The android-client uses Flutter and has a rough and simple UI. The Android app's audio capture and UDP streaming logic are written in C++ to lower latency, with the former using [Google's Oboe](https://github.com/google/oboe) Library. View [`audio_router.cpp`](https://github.com/nullptr0x/MicrophoneRouter/blob/main/microphone_router/src/audio_router.cpp) if you are interested in the implementation.

If you want to do the same, here are the steps which worked for me on my Fedora 44 (PipeWire and WirePlumber, with gstreamer):
```bash
$ hostname -I  # fetch your device's local IP
```
Use the IP you fectched above and enter it in the android client. Then choose your preferred port and press the microphone icon button to start the transmission.

On your computer, verify whether you are receivng the data on your selected port via netcat: 
```bash
$ nc -ul -p <port> | xxd
```
If you don't see anything, check your firewall configuration and make it allow UDP traffic on the port. If it works, we proceed to virtual device creation.
```bash
$ pw-loopback --name="Virtual_Mic" --capture-props="media.class=Audio/Sink node.name=Virtual_Mic_In node.description='Virtual Mic Input'" --playback-props="media.class=Audio/Source node.name=Virtual_Mic node.description='Virtual_Mic'"
```
This creates a virtutal mic which your apps may recognize as "Virtual_Mic". We now need to feed our raw UDP data to it, for which I used `gstreamer`:
```bash
$ gst-launch-1.0 udpsrc port=5000 ! \
        "audio/x-raw,format=F32LE,rate=48000,channels=1,layout=interleaved" ! \
        audioconvert ! audioresample ! \
        pipewiresink target-object="Virtual_Mic_In"
```