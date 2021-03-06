#include "sources/audiosource.h"

#include "util/logger.h"

namespace mixxx {

namespace {

const Logger kLogger("AudioSource");

} // anonymous namespace

AudioSource::AudioSource(QUrl url)
        : UrlResource(url),
          AudioSignal(kSampleLayout) {
}

AudioSource::OpenResult AudioSource::open(
        OpenMode mode,
        const OpenParams& params) {
    close(); // reopening is not supported

    OpenResult result;
    try {
        result = tryOpen(mode, params);
    } catch (const std::exception& e) {
        qWarning() << "Caught unexpected exception from SoundSource::tryOpen():" << e.what();
        result = OpenResult::Failed;
    } catch (...) {
        qWarning() << "Caught unknown exception from SoundSource::tryOpen()";
        result = OpenResult::Failed;
    }
    if (OpenResult::Succeeded != result) {
        close(); // rollback
    }
    return result;
}

bool AudioSource::initFrameIndexRangeOnce(
        IndexRange frameIndexRange) {
    VERIFY_OR_DEBUG_ASSERT(frameIndexRange.orientation() != IndexRange::Orientation::Backward) {
        kLogger.warning()
                << "Backward frame index range not supported"
                << frameIndexRange;
        return false; // abort
    }
    VERIFY_OR_DEBUG_ASSERT(m_frameIndexRange.empty() || (m_frameIndexRange == frameIndexRange)) {
        kLogger.warning()
                << "Frame index range has already been initialized to"
                << m_frameIndexRange
                << "which differs from"
                << frameIndexRange;
        return false; // abort
    }
    m_frameIndexRange = frameIndexRange;
    return true;
}

bool AudioSource::initBitrateOnce(Bitrate bitrate) {
    if (bitrate < Bitrate()) {
        kLogger.warning()
                << "Invalid bitrate"
                << bitrate;
        return false; // abort
    }
    VERIFY_OR_DEBUG_ASSERT(!m_bitrate.valid() || (m_bitrate == bitrate)) {
        kLogger.warning()
                << "Bitrate has already been initialized to"
                << m_bitrate
                << "which differs from"
                << bitrate;
        return false; // abort
    }
    m_bitrate = bitrate;
    return true;
}

bool AudioSource::verifyReadable() const {
    bool result = AudioSignal::verifyReadable();
    if (frameIndexRange().empty()) {
        kLogger.warning()
                << "No audio data available";
        // Don't set the result to false, even if reading from an empty source
        // is pointless!
    }
    if (m_bitrate != Bitrate()) {
        VERIFY_OR_DEBUG_ASSERT(m_bitrate.valid()) {
            kLogger.warning()
                    << "Invalid bitrate [kbps]:"
                    << m_bitrate;
            // Don't set the result to false, because bitrate is only
            // an informational property that does not effect the ability
            // to decode audio data!
        }
    }
    return result;
}

WritableSampleFrames AudioSource::clampWritableSampleFrames(
        WritableSampleFrames sampleFrames) const {
    const auto readableFrameIndexRange =
            clampFrameIndexRange(sampleFrames.frameIndexRange());
    // adjust offset and length of the sample buffer
    DEBUG_ASSERT(sampleFrames.frameIndexRange().start() <= readableFrameIndexRange.end());
    auto writableFrameIndexRange =
            IndexRange::between(sampleFrames.frameIndexRange().start(), readableFrameIndexRange.end());
    const SINT minSampleBufferCapacity =
            frames2samples(writableFrameIndexRange.length());
    VERIFY_OR_DEBUG_ASSERT(sampleFrames.writableLength() >= minSampleBufferCapacity) {
        kLogger.critical()
                << "Capacity of output buffer is too small"
                << sampleFrames.writableLength()
                << "<"
                << minSampleBufferCapacity
                << "to store all readable sample frames"
                << readableFrameIndexRange
                << "into writable sample frames"
                << writableFrameIndexRange;
        writableFrameIndexRange =
                writableFrameIndexRange.splitAndShrinkFront(
                        samples2frames(sampleFrames.writableLength()));
        kLogger.warning()
                << "Reduced writable sample frames"
                << writableFrameIndexRange;
    }
    DEBUG_ASSERT(readableFrameIndexRange.start() >= writableFrameIndexRange.start());
    const SINT writableFrameOffset =
            readableFrameIndexRange.start() - writableFrameIndexRange.start();
    writableFrameIndexRange.shrinkFront(writableFrameOffset);
    return WritableSampleFrames(
            writableFrameIndexRange,
            SampleBuffer::WritableSlice(
                    sampleFrames.writableData(frames2samples(writableFrameOffset)),
                    frames2samples(writableFrameIndexRange.length())));
}

ReadableSampleFrames AudioSource::readSampleFrames(
        WritableSampleFrames sampleFrames) {
    const auto writable =
            clampWritableSampleFrames(sampleFrames);
    if (writable.frameIndexRange().empty()) {
        // result is empty
        return ReadableSampleFrames(writable.frameIndexRange());
    } else {
        // forward clamped request
        ReadableSampleFrames readable = readSampleFramesClamped(writable);
        DEBUG_ASSERT(readable.frameIndexRange().empty() ||
                readable.frameIndexRange() <= writable.frameIndexRange());
        if (readable.frameIndexRange() != writable.frameIndexRange()) {
            kLogger.warning()
                    << "Failed to read sample frames:"
                    << "expected =" << writable.frameIndexRange()
                    << ", actual =" << readable.frameIndexRange();
            auto shrinkedFrameIndexRange = m_frameIndexRange;
            if (readable.frameIndexRange().empty()) {
                // Adjust upper bound: Consider all audio data following
                // the read position until the end as unreadable
                shrinkedFrameIndexRange.shrinkBack(
                        shrinkedFrameIndexRange.end() - writable.frameIndexRange().start());
            } else {
                // Adjust lower bound of readable audio data
                if (writable.frameIndexRange().start() < readable.frameIndexRange().start()) {
                    shrinkedFrameIndexRange.shrinkFront(
                            readable.frameIndexRange().start() - shrinkedFrameIndexRange.start());
                }
                // Adjust upper bound of readable audio data
                if (writable.frameIndexRange().end() > readable.frameIndexRange().end()) {
                    shrinkedFrameIndexRange.shrinkBack(
                            shrinkedFrameIndexRange.end() - readable.frameIndexRange().end());
                }
            }
            DEBUG_ASSERT(shrinkedFrameIndexRange < m_frameIndexRange);
            kLogger.info()
                    << "Shrinking readable frame index range:"
                    << "before =" << m_frameIndexRange
                    << ", after =" << shrinkedFrameIndexRange;
            // Propagate the adjustments to all participants in the
            // inheritance hierarchy.
            // NOTE(2019-08-31, uklotzde): This is an ugly hack to overcome
            // the previous assumption that the frame index range is immutable
            // for the whole lifetime of an AudioSource. As we know now it is
            // not and for a future re-design we need to account for this fact!!
            adjustFrameIndexRange(shrinkedFrameIndexRange);
        }
        return readable;
    }
}

void AudioSource::adjustFrameIndexRange(
        IndexRange frameIndexRange) {
    DEBUG_ASSERT(frameIndexRange <= m_frameIndexRange);
    m_frameIndexRange = frameIndexRange;
}

} // namespace mixxx
