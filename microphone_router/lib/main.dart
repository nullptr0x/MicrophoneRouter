import 'package:microphone_router/audio_bridge.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:flutter/material.dart';


void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
    const MyApp({super.key});
    
    @override
    Widget build(BuildContext context) {
        return const MaterialApp(
            home: HomeScreen()
        );
    }
}

class HomeScreen extends StatefulWidget {
    const HomeScreen({super.key});

    @override
    State<HomeScreen> createState() => _HomeScreenState();
}


class _HomeScreenState extends State<HomeScreen> {
    bool _isWorking = false;

    late TextEditingController _IPController;
    late TextEditingController _PortController;
    
    final AudioBridge _audioBridge = AudioBridge();


    @override
    void initState() {
        super.initState();
        _IPController = TextEditingController(text: "192.168.29.133");
        _PortController = TextEditingController(text: "5000");
    }
    

    @override
    void dispose() {
        _audioBridge.dispose();
        _IPController.dispose();
        _PortController.dispose();
        super.dispose();
    }


    void _toggleAudioStream() async {
        if (_isWorking) {
            _audioBridge.stop();
        } else {
            var status = await Permission.microphone.request();
            if (!status.isGranted) return;

            final String inputIP  = _IPController.text;
            final int   inputPort = int.tryParse(_PortController.text) ?? 5000;
            
            final int result = _audioBridge.initialize(inputIP, inputPort);
        }
        
        setState(() {
            _isWorking = !_isWorking;
        });
    }

    @override
    Widget build(BuildContext context) {
        return Scaffold(
            appBar: AppBar(
                title: const Text("Microphone Router"),
                backgroundColor: Colors.blue,
            ),

            body: Padding 
            (
                padding: EdgeInsets.all(16.0),
                child: Center(
                    child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: <Widget>[
                            TextField(
                                controller: _IPController,
                                decoration: const InputDecoration(
                                    labelText: "Target IPv4 Address",
                                    border: OutlineInputBorder(),
                                ),

                                keyboardType: TextInputType.values[1],
                            ),

                            const SizedBox(height: 32.0),

                            TextField(
                                controller: _PortController,
                                decoration: const InputDecoration(
                                    labelText: "Port",
                                    border: OutlineInputBorder(),
                                ),
                            ),

                            const SizedBox(height: 32.0,),

                            Text(
                                _isWorking ? "ROUTING ACTIVE" : "ROUTING PAUSED",
                                style: TextStyle(
                                    fontSize: 22.0,
                                    fontWeight: FontWeight.bold,
                                    color: _isWorking ? Colors.green : Colors.grey,
                                ),), 
                                
                                const SizedBox(height: 32.0),

                                IconButton(
                                    onPressed: _toggleAudioStream, 
                                    iconSize: 80.0,
                                    icon: Icon(
                                        _isWorking ? Icons.mic : Icons.mic_off,
                                        color: _isWorking ? Colors.green : Colors.grey
                                    )),
                        ]
                    ),
                )
            ),
        );
    }
}
