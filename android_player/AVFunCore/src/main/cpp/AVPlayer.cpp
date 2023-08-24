#include "AVPlayer.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "LogUtil.h"
#include "Utils.h"

#include "GLContext.h"

namespace avf {

    int AVPlayer::OpenVideo(std::string filename) {
        LOG_INFO("filename: %s", filename.c_str());
        extractor = AMediaExtractor_new();
//        media_status_t err = AMediaExtractor_setDataSource(extractor, filename.c_str());

        int fd = open(filename.c_str(), O_RDONLY);
        if(fd == -1){
            LOG_ERROR("open error: %s", strerror(fd));
            return -1;
        }

        off64_t len = lseek(fd, 0, SEEK_END);

        lseek(fd, 0, SEEK_SET);

        media_status_t err = AMediaExtractor_setDataSourceFd(extractor, fd, 0, len);

        if (err != AMEDIA_OK) {
            LOG_ERROR("setDataSource error: %d", err);
            return -1;
        }

        int numtracks = AMediaExtractor_getTrackCount(extractor);

        LOG_INFO("input has %d tracks", numtracks);
        for (int i = 0; i < numtracks; i++) {
            AMediaFormat *format = AMediaExtractor_getTrackFormat(extractor, i);
            const char *s = AMediaFormat_toString(format);
            LOG_INFO("track %d format: %s", i, s);
            const char *mime;
            if (!AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)) {
                LOG_INFO("no mime type");
                return -1;
            } else if (!strncmp(mime, "video/", 6)) {
                // Omitting most error handling for clarity.
                // Production code should check for errors.
                AMediaExtractor_selectTrack(extractor, i);
                videoCodec = AMediaCodec_createDecoderByType(mime);
                AMediaCodec_configure(videoCodec, format, NULL, NULL, 0);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);

                LOG_INFO("WxH=[%dx%d]",width,height);
                fmtUnit = 3 / 2;
                videoFrameBuffer = std::make_shared<VideoFrameBuffer>(width, height, fmtUnit, 16);

                AMediaCodec_start(videoCodec);
            } else if (!strncmp(mime, "audio/", 6)) {
//            AMediaExtractor_selectTrack(extractor, i);
//            audioCodec = AMediaCodec_createDecoderByType(mime);
//            AMediaCodec_configure(audioCodec, format, NULL, NULL, 0);
//            AMediaCodec_start(audioCodec);
            }

            LOG_INFO("mime: %s", mime);
            AMediaFormat_delete(format);
        }

        th_videoDecode = std::thread(&AVPlayer::readSample, this);
        th_audioDecode = std::thread(&AVPlayer::audioDecode, this);

        close(fd);

        return 0;
    }

    void AVPlayer::readSample() {
        ssize_t bufidx = -1;

        while (true) {

            if (intercepted) {
                break;
            }

            if (sawOutputEOS) {
                usleep(10 * 1000);
                continue;
            }

            AMediaCodecBufferInfo info;
            auto status = AMediaCodec_dequeueOutputBuffer(videoCodec, &info, 0);

            if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
                if (!sawInputEOS) {
                    bufidx = AMediaCodec_dequeueInputBuffer(videoCodec, 2000);
                    LOG_DEBUG("input buffer %zd", bufidx);
                    if (bufidx >= 0) {
                        size_t bufsize;
                        auto buf = AMediaCodec_getInputBuffer(videoCodec, bufidx, &bufsize);
                        auto sampleSize = AMediaExtractor_readSampleData(extractor, buf, bufsize);
                        if (sampleSize < 0) {
                            sampleSize = 0;
                            sawInputEOS = true;
                            LOG_DEBUG("EOS");
                        }

                        auto presentationTimeUs = AMediaExtractor_getSampleTime(extractor);

                        AMediaCodec_queueInputBuffer(
                                videoCodec, bufidx, 0, sampleSize, presentationTimeUs,
                                sawInputEOS ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM : 0);
                        AMediaExtractor_advance(extractor);

                    }
                }
                continue;
            }

            if (status >= 0) {
                if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                    LOG_DEBUG("output EOS");
                    sawOutputEOS = true;
                }

                int64_t presentationNano = info.presentationTimeUs * 1000;

                size_t out_size = 0;
                uint8_t *out_buf = AMediaCodec_getOutputBuffer(videoCodec, status, &out_size);

                LOG_DEBUG("size: %lld [%d, %d]-------- %lld", out_size, info.offset, info.size, presentationNano);
#if 0
                {
                    static int count = 0;
                    char srcfn[1024];
                    sprintf(srcfn,"/data/user/0/com.example.avfunplayer/cache/test_%d.yuv",count++);
                    FILE *fdw = fopen(srcfn, "wb+");
                    if(fdw == nullptr){
                        LOG_ERROR("open error: %s", strerror(errno));
                    }else {

                        fwrite(out_buf + info.offset, 1, info.size, fdw);

                        fclose(fdw);
                    }
                }
#endif

                char *write_buf = videoFrameBuffer->writable(out_size);

                if (write_buf != nullptr) {
                    int size = width * height * fmtUnit;
                    if(size <= info.size){
                        memcpy(write_buf + info.offset, out_buf, size);
                        videoFrameBuffer->push(out_size);
                    }else{
                        LOG_ERROR("size: %d, info.size: %d",size, info.size);
                    }
                }

                AMediaCodec_releaseOutputBuffer(videoCodec, status, info.size != 0);

            } else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
                LOG_DEBUG("output buffers changed");
            } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                auto format = AMediaCodec_getOutputFormat(videoCodec);
                LOG_DEBUG("format changed to: %s", AMediaFormat_toString(format));
                AMediaFormat_delete(format);
            } else {
                LOG_DEBUG("unexpected info code: %zd", status);
            }

            usleep(1000);
        }

    }

    void AVPlayer::audioDecode() {

    }

    void AVPlayer::initGL(int screenWidth,int screenHeight){

        constexpr auto _vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

        constexpr auto _fragment_shader = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";


        //AVFSizei size = {1202,720};

        auto vertices = Utils::FitIn(screenWidth, screenHeight, width, height);

        program = make_up<render::GLProgram>(_vertex_shader, _fragment_shader);

        int indices[] = {
                0, 1, 2,
                0, 2, 3,
        };

        glGenVertexArrays(1, &VAO);

        unsigned int VBO;
        glGenBuffers(1, &VBO);

        unsigned int EBO;
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        //checkGLError(__LINE__);
    }

    int AVPlayer::renderVideo() {

        GLContext::GetInstance()->Make();


        auto screenWidth = GLContext::GetInstance()->GetScreenWidth();
        auto screenHeight = GLContext::GetInstance()->GetScreenHeight();

        initGL(screenWidth, screenHeight);

        while (true)
        {
            if(intercepted)
            {
                break;
            }

            glClearColor(0, 1.0f, 0, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);


            GLContext::GetInstance()->Swap();
            usleep(30 * 100);
        }

        return 0;
    }

    int AVPlayer::Start() {
        renderVideo();

        return 0;
    }


    int AVPlayer::Stop() {
        intercepted = true;

        return 0;
    }

}

