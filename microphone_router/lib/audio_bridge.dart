import 'dart:ffi';
import 'package:ffi/ffi.dart';

typedef InitAudioEngineC = Int32 Function(Pointer<Utf8> ip, Int32 port);
typedef InitAudioEngineDart = int Function(Pointer<Utf8> ip, int port);

typedef StopAudioEngineC = Void Function();
typedef StopAudioEngineDart = void Function();


class AudioBridge {
    late final DynamicLibrary _nativeLib;
    late final InitAudioEngineDart _initAudioEngine;
    late final StopAudioEngineDart _stopAudioEngine;


    AudioBridge() {
        _nativeLib = DynamicLibrary.open('libaudio_router.so');

        _initAudioEngine = _nativeLib
            .lookup<NativeFunction<InitAudioEngineC>>('init_audio_engine')
            .asFunction<InitAudioEngineDart>();

        _stopAudioEngine = _nativeLib
            .lookup<NativeFunction<StopAudioEngineC>>('stop_audio_engine')
            .asFunction<StopAudioEngineDart>();
    }


    int initialize(String ip, int port) {
        final Pointer<Utf8> ipPointer = ip.toNativeUtf8();
        final int result = _initAudioEngine(ipPointer, port);
        calloc.free(ipPointer);
        return result;
    }


    void stop() {
        _stopAudioEngine();
    }


    void dispose() {
        stop();
    }
}
