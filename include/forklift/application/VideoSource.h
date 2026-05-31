// Port: VideoSource — produces frames from any source (RTSP, file, USB, mock).

#ifndef FORKLIFT_APPLICATION_VIDEO_SOURCE_H_
#define FORKLIFT_APPLICATION_VIDEO_SOURCE_H_

#include <optional>
#include <string>

#include "forklift/domain/Frame.h"
#include "forklift/shared/Result.h"

namespace forklift::application {

class VideoSource {
public:
    virtual ~VideoSource() = default;

    virtual shared::Result<void>            open()                 = 0;
    virtual void                            close()                = 0;
    [[nodiscard]] virtual bool              is_open() const        = 0;
    [[nodiscard]] virtual std::optional<domain::Frame> read_frame() = 0;
    [[nodiscard]] virtual const std::string& camera_id() const     = 0;
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_VIDEO_SOURCE_H_
