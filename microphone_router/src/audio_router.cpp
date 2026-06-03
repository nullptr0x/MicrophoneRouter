#include <cstdint>
#include <cstdio>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <string>

#include <oboe/Oboe.h>
#include <oboe/AudioStreamCallback.h>
#include <oboe/FifoBuffer.h>
#include <oboe/AudioStreamBuilder.h>

#include <android/log.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>


#define LOG_TAG "AudioRoutingEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


class AudioRoutingEngine : oboe::AudioStreamDataCallback {
public:
    AudioRoutingEngine(const char* ip, std::int32_t port)
        : m_IP(ip), m_Port(port) {}


    bool start() {
        if (m_IsPlaying) {
            return true;
        }

        // initialize the UDP socket
        m_SocketFd = socket(AF_INET, SOCK_DGRAM, 0);

        m_TargetAddress.sin_family = AF_INET;
        m_TargetAddress.sin_port   = htons(m_Port);

        if (inet_pton(AF_INET, m_IP.c_str(), &m_TargetAddress.sin_addr) <= 0) {
            close(m_SocketFd);
            m_SocketFd = -1;
            LOGE("Failed to parse the IP address.");
            return false;  // failed to parse address
        }

        oboe::AudioStreamBuilder builder;
        builder.setDirection(oboe::Direction::Input)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setDataCallback(this)
            ->setSampleRate(48000)
            ->setChannelCount(1)
            ->setFormat(oboe::AudioFormat::Float);

        oboe::Result result = builder.openStream(m_Stream);
        if (result != oboe::Result::OK) {
            close(m_SocketFd);
            m_SocketFd = -1;
            LOGE("start: failed to open the audio stream");
            return false;
        }  // unsuccessful

        // compute the necessary details for the FIFO buffer
        std::int32_t native_channel_count = m_Stream->getChannelCount();
        std::int32_t fifo_capacity        = m_Stream->getFramesPerBurst() * 16;

        m_BytesPerFrame  = m_Stream->getBytesPerFrame();
        // aim for packets around 1400 bytes to avoid fragmentation
        m_FramesPerBatch = 1400 / m_BytesPerFrame;

        m_FIFOBuffer = std::make_unique<oboe::FifoBuffer>(m_BytesPerFrame, fifo_capacity);

        m_IsPlaying = true;
        m_WorkerThread = std::thread([this] {
            sendData();
        });

        m_Stream->requestStart();
        return true;
    }


    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream* audioStream,
            void* audioData,
            std::int32_t numFrames) override
    {
        if (audioStream->getDirection() == oboe::Direction::Input) {
            m_FIFOBuffer->write(audioData, numFrames);
        }

        return oboe::DataCallbackResult::Continue;
    }


    void stop() {
        m_IsPlaying = false;
        if (m_WorkerThread.joinable()) {
            m_WorkerThread.join();
        }

        if (m_Stream) {
            m_Stream->stop();
            m_Stream->close();
        }

        if (m_SocketFd >= 0) {
            close(m_SocketFd);
            m_SocketFd = -1;
        }
    }


    /// Responsible for sending the raw data to the LAN device
    void sendData() {
        auto buffer = std::vector<uint8_t>(m_BytesPerFrame * m_FramesPerBatch);
        while (m_IsPlaying) {
            int32_t frames = m_FIFOBuffer->read(buffer.data(), m_FramesPerBatch);
            if (frames > 0) {
                sendto(m_SocketFd, buffer.data(),
                       frames * m_BytesPerFrame, 0,
                       reinterpret_cast<sockaddr*>(&m_TargetAddress),
                       sizeof(m_TargetAddress));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }


    ~AudioRoutingEngine() override {
        stop();
    }


private:
    std::atomic<bool> m_IsPlaying = false;
    std::shared_ptr<oboe::AudioStream> m_Stream;
    std::thread m_WorkerThread;
    std::unique_ptr<oboe::FifoBuffer> m_FIFOBuffer = nullptr;

    std::string  m_IP;
    std::int32_t m_Port;

    // socket stuff
    int m_SocketFd{-1};
    sockaddr_in m_TargetAddress{};

    std::int32_t m_BytesPerFrame;  // new
    std::int32_t m_FramesPerBatch; // new
};


AudioRoutingEngine* AudioEngine = nullptr;


extern "C" {
    std::int32_t init_audio_engine(const char* ip, std::int32_t port) {
        if (!AudioEngine) {
            AudioEngine = new AudioRoutingEngine(ip, port);
        }

        if (AudioEngine->start()) {
            return 1;
        }

        return 0;
    }


    void stop_audio_engine() {
        if (AudioEngine) {
            AudioEngine->stop();
            delete AudioEngine;
            AudioEngine = nullptr;
        }
    }
}