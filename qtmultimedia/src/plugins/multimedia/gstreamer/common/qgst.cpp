// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgst_p.h>
#include <common/qgst_debug_p.h>
#include <common/qgstpipeline_p.h>
#include <common/qgstreamermessage_p.h>

#include <QtCore/qdebug.h>
#include <QtMultimedia/qcameradevice.h>

#include <array>

QT_BEGIN_NAMESPACE

namespace {

struct VideoFormat
{
    QVideoFrameFormat::PixelFormat pixelFormat;
    GstVideoFormat gstFormat;
};

constexpr std::array<VideoFormat, 19> qt_videoFormatLookup{ {
        { QVideoFrameFormat::Format_YUV420P, GST_VIDEO_FORMAT_I420 },
        { QVideoFrameFormat::Format_YUV422P, GST_VIDEO_FORMAT_Y42B },
        { QVideoFrameFormat::Format_YV12, GST_VIDEO_FORMAT_YV12 },
        { QVideoFrameFormat::Format_UYVY, GST_VIDEO_FORMAT_UYVY },
        { QVideoFrameFormat::Format_YUYV, GST_VIDEO_FORMAT_YUY2 },
        { QVideoFrameFormat::Format_NV12, GST_VIDEO_FORMAT_NV12 },
        { QVideoFrameFormat::Format_NV21, GST_VIDEO_FORMAT_NV21 },
        { QVideoFrameFormat::Format_AYUV, GST_VIDEO_FORMAT_AYUV },
        { QVideoFrameFormat::Format_Y8, GST_VIDEO_FORMAT_GRAY8 },
        { QVideoFrameFormat::Format_XRGB8888, GST_VIDEO_FORMAT_xRGB },
        { QVideoFrameFormat::Format_XBGR8888, GST_VIDEO_FORMAT_xBGR },
        { QVideoFrameFormat::Format_RGBX8888, GST_VIDEO_FORMAT_RGBx },
        { QVideoFrameFormat::Format_BGRX8888, GST_VIDEO_FORMAT_BGRx },
        { QVideoFrameFormat::Format_ARGB8888, GST_VIDEO_FORMAT_ARGB },
        { QVideoFrameFormat::Format_ABGR8888, GST_VIDEO_FORMAT_ABGR },
        { QVideoFrameFormat::Format_RGBA8888, GST_VIDEO_FORMAT_RGBA },
        { QVideoFrameFormat::Format_BGRA8888, GST_VIDEO_FORMAT_BGRA },
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        { QVideoFrameFormat::Format_Y16, GST_VIDEO_FORMAT_GRAY16_LE },
        { QVideoFrameFormat::Format_P010, GST_VIDEO_FORMAT_P010_10LE },
#else
        { QVideoFrameFormat::Format_Y16, GST_VIDEO_FORMAT_GRAY16_BE },
        { QVideoFrameFormat::Format_P010, GST_VIDEO_FORMAT_P010_10BE },
#endif
} };

int indexOfVideoFormat(QVideoFrameFormat::PixelFormat format)
{
    for (size_t i = 0; i < qt_videoFormatLookup.size(); ++i)
        if (qt_videoFormatLookup[i].pixelFormat == format)
            return int(i);

    return -1;
}

int indexOfVideoFormat(GstVideoFormat format)
{
    for (size_t i = 0; i < qt_videoFormatLookup.size(); ++i)
        if (qt_videoFormatLookup[i].gstFormat == format)
            return int(i);

    return -1;
}

} // namespace

// QGValue

QGValue::QGValue(const GValue *v) : value(v) { }

bool QGValue::isNull() const
{
    return !value;
}

std::optional<bool> QGValue::toBool() const
{
    if (!G_VALUE_HOLDS_BOOLEAN(value))
        return std::nullopt;
    return g_value_get_boolean(value);
}

std::optional<int> QGValue::toInt() const
{
    if (!G_VALUE_HOLDS_INT(value))
        return std::nullopt;
    return g_value_get_int(value);
}

std::optional<int> QGValue::toInt64() const
{
    if (!G_VALUE_HOLDS_INT64(value))
        return std::nullopt;
    return g_value_get_int64(value);
}

const char *QGValue::toString() const
{
    return value ? g_value_get_string(value) : nullptr;
}

std::optional<float> QGValue::getFraction() const
{
    if (!GST_VALUE_HOLDS_FRACTION(value))
        return std::nullopt;
    return (float)gst_value_get_fraction_numerator(value)
            / (float)gst_value_get_fraction_denominator(value);
}

std::optional<QGRange<float>> QGValue::getFractionRange() const
{
    if (!GST_VALUE_HOLDS_FRACTION_RANGE(value))
        return std::nullopt;
    QGValue min = QGValue{ gst_value_get_fraction_range_min(value) };
    QGValue max = QGValue{ gst_value_get_fraction_range_max(value) };
    return QGRange<float>{ *min.getFraction(), *max.getFraction() };
}

std::optional<QGRange<int>> QGValue::toIntRange() const
{
    if (!GST_VALUE_HOLDS_INT_RANGE(value))
        return std::nullopt;
    return QGRange<int>{ gst_value_get_int_range_min(value), gst_value_get_int_range_max(value) };
}

QGstStructure QGValue::toStructure() const
{
    if (!value || !GST_VALUE_HOLDS_STRUCTURE(value))
        return QGstStructure();
    return QGstStructure(gst_value_get_structure(value));
}

QGstCaps QGValue::toCaps() const
{
    if (!value || !GST_VALUE_HOLDS_CAPS(value))
        return {};
    return QGstCaps(gst_caps_copy(gst_value_get_caps(value)), QGstCaps::HasRef);
}

bool QGValue::isList() const
{
    return value && GST_VALUE_HOLDS_LIST(value);
}

int QGValue::listSize() const
{
    return gst_value_list_get_size(value);
}

QGValue QGValue::at(int index) const
{
    return QGValue{ gst_value_list_get_value(value, index) };
}

// QGstStructure

QGstStructure::QGstStructure(const GstStructure *s) : structure(s) { }

void QGstStructure::free()
{
    if (structure)
        gst_structure_free(const_cast<GstStructure *>(structure));
    structure = nullptr;
}

bool QGstStructure::isNull() const
{
    return !structure;
}

QByteArrayView QGstStructure::name() const
{
    return gst_structure_get_name(structure);
}

QGValue QGstStructure::operator[](const char *name) const
{
    return QGValue{ gst_structure_get_value(structure, name) };
}

QGstStructure QGstStructure::copy() const
{
    return gst_structure_copy(structure);
}

QSize QGstStructure::resolution() const
{
    QSize size;

    int w, h;
    if (structure && gst_structure_get_int(structure, "width", &w)
        && gst_structure_get_int(structure, "height", &h)) {
        size.rwidth() = w;
        size.rheight() = h;
    }

    return size;
}

QVideoFrameFormat::PixelFormat QGstStructure::pixelFormat() const
{
    QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;

    if (!structure)
        return pixelFormat;

    if (gst_structure_has_name(structure, "video/x-raw")) {
        const gchar *s = gst_structure_get_string(structure, "format");
        if (s) {
            GstVideoFormat format = gst_video_format_from_string(s);
            int index = indexOfVideoFormat(format);

            if (index != -1)
                pixelFormat = qt_videoFormatLookup[index].pixelFormat;
        }
    } else if (gst_structure_has_name(structure, "image/jpeg")) {
        pixelFormat = QVideoFrameFormat::Format_Jpeg;
    }

    return pixelFormat;
}

QGRange<float> QGstStructure::frameRateRange() const
{
    float minRate = 0.;
    float maxRate = 0.;

    if (!structure)
        return { 0.f, 0.f };

    auto extractFraction = [](const GValue *v) -> float {
        return (float)gst_value_get_fraction_numerator(v)
                / (float)gst_value_get_fraction_denominator(v);
    };
    auto extractFrameRate = [&](const GValue *v) {
        auto insert = [&](float min, float max) {
            if (max > maxRate)
                maxRate = max;
            if (min < minRate)
                minRate = min;
        };

        if (GST_VALUE_HOLDS_FRACTION(v)) {
            float rate = extractFraction(v);
            insert(rate, rate);
        } else if (GST_VALUE_HOLDS_FRACTION_RANGE(v)) {
            auto *min = gst_value_get_fraction_range_max(v);
            auto *max = gst_value_get_fraction_range_max(v);
            insert(extractFraction(min), extractFraction(max));
        }
    };

    const GValue *gstFrameRates = gst_structure_get_value(structure, "framerate");
    if (gstFrameRates) {
        if (GST_VALUE_HOLDS_LIST(gstFrameRates)) {
            guint nFrameRates = gst_value_list_get_size(gstFrameRates);
            for (guint f = 0; f < nFrameRates; ++f) {
                extractFrameRate(gst_value_list_get_value(gstFrameRates, f));
            }
        } else {
            extractFrameRate(gstFrameRates);
        }
    } else {
        const GValue *min = gst_structure_get_value(structure, "min-framerate");
        const GValue *max = gst_structure_get_value(structure, "max-framerate");
        if (min && max) {
            minRate = extractFraction(min);
            maxRate = extractFraction(max);
        }
    }

    return { minRate, maxRate };
}

QGstreamerMessage QGstStructure::getMessage()
{
    GstMessage *message = nullptr;
    gst_structure_get(structure, "message", GST_TYPE_MESSAGE, &message, nullptr);
    return QGstreamerMessage(message, QGstreamerMessage::HasRef);
}

std::optional<Fraction> QGstStructure::pixelAspectRatio() const
{
    gint numerator;
    gint denominator;
    if (gst_structure_get_fraction(structure, "pixel-aspect-ratio", &numerator, &denominator)) {
        return Fraction{
            numerator,
            denominator,
        };
    }

    return std::nullopt;
}

QSize QGstStructure::nativeSize() const
{
    QSize size = resolution();
    if (!size.isValid()) {
        qWarning() << Q_FUNC_INFO << "invalid resolution when querying nativeSize";
        return size;
    }

    std::optional<Fraction> par = pixelAspectRatio();
    if (par)
        size = qCalculateFrameSize(size, *par);
    return size;
}

// QGstCaps

std::optional<std::pair<QVideoFrameFormat, GstVideoInfo>> QGstCaps::formatAndVideoInfo() const
{
    GstVideoInfo vidInfo;

    bool success = gst_video_info_from_caps(&vidInfo, get());
    if (!success)
        return std::nullopt;

    int index = indexOfVideoFormat(vidInfo.finfo->format);
    if (index == -1)
        return std::nullopt;

    QVideoFrameFormat format(QSize(vidInfo.width, vidInfo.height),
                             qt_videoFormatLookup[index].pixelFormat);

    if (vidInfo.fps_d > 0)
        format.setFrameRate(qreal(vidInfo.fps_n) / vidInfo.fps_d);

    QVideoFrameFormat::ColorRange range = QVideoFrameFormat::ColorRange_Unknown;
    switch (vidInfo.colorimetry.range) {
    case GST_VIDEO_COLOR_RANGE_UNKNOWN:
        break;
    case GST_VIDEO_COLOR_RANGE_0_255:
        range = QVideoFrameFormat::ColorRange_Full;
        break;
    case GST_VIDEO_COLOR_RANGE_16_235:
        range = QVideoFrameFormat::ColorRange_Video;
        break;
    }
    format.setColorRange(range);

    QVideoFrameFormat::ColorSpace colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
    switch (vidInfo.colorimetry.matrix) {
    case GST_VIDEO_COLOR_MATRIX_UNKNOWN:
    case GST_VIDEO_COLOR_MATRIX_RGB:
    case GST_VIDEO_COLOR_MATRIX_FCC:
        break;
    case GST_VIDEO_COLOR_MATRIX_BT709:
        colorSpace = QVideoFrameFormat::ColorSpace_BT709;
        break;
    case GST_VIDEO_COLOR_MATRIX_BT601:
        colorSpace = QVideoFrameFormat::ColorSpace_BT601;
        break;
    case GST_VIDEO_COLOR_MATRIX_SMPTE240M:
        colorSpace = QVideoFrameFormat::ColorSpace_AdobeRgb;
        break;
    case GST_VIDEO_COLOR_MATRIX_BT2020:
        colorSpace = QVideoFrameFormat::ColorSpace_BT2020;
        break;
    }
    format.setColorSpace(colorSpace);

    QVideoFrameFormat::ColorTransfer transfer = QVideoFrameFormat::ColorTransfer_Unknown;
    switch (vidInfo.colorimetry.transfer) {
    case GST_VIDEO_TRANSFER_UNKNOWN:
        break;
    case GST_VIDEO_TRANSFER_GAMMA10:
        transfer = QVideoFrameFormat::ColorTransfer_Linear;
        break;
    case GST_VIDEO_TRANSFER_GAMMA22:
    case GST_VIDEO_TRANSFER_SMPTE240M:
    case GST_VIDEO_TRANSFER_SRGB:
    case GST_VIDEO_TRANSFER_ADOBERGB:
        transfer = QVideoFrameFormat::ColorTransfer_Gamma22;
        break;
    case GST_VIDEO_TRANSFER_GAMMA18:
    case GST_VIDEO_TRANSFER_GAMMA20:
        // not quite, but best fit
    case GST_VIDEO_TRANSFER_BT709:
    case GST_VIDEO_TRANSFER_BT2020_12:
        transfer = QVideoFrameFormat::ColorTransfer_BT709;
        break;
    case GST_VIDEO_TRANSFER_GAMMA28:
        transfer = QVideoFrameFormat::ColorTransfer_Gamma28;
        break;
    case GST_VIDEO_TRANSFER_LOG100:
    case GST_VIDEO_TRANSFER_LOG316:
        break;
#if GST_CHECK_VERSION(1, 18, 0)
    case GST_VIDEO_TRANSFER_SMPTE2084:
        transfer = QVideoFrameFormat::ColorTransfer_ST2084;
        break;
    case GST_VIDEO_TRANSFER_ARIB_STD_B67:
        transfer = QVideoFrameFormat::ColorTransfer_STD_B67;
        break;
    case GST_VIDEO_TRANSFER_BT2020_10:
        transfer = QVideoFrameFormat::ColorTransfer_BT709;
        break;
    case GST_VIDEO_TRANSFER_BT601:
        transfer = QVideoFrameFormat::ColorTransfer_BT601;
        break;
#endif
    }
    format.setColorTransfer(transfer);

    return std::pair{
        std::move(format),
        vidInfo,
    };
}

void QGstCaps::addPixelFormats(const QList<QVideoFrameFormat::PixelFormat> &formats,
                               const char *modifier)
{
    if (!gst_caps_is_writable(get()))
        *this = QGstCaps(gst_caps_make_writable(release()), QGstCaps::RefMode::HasRef);

    GValue list = {};
    g_value_init(&list, GST_TYPE_LIST);

    for (QVideoFrameFormat::PixelFormat format : formats) {
        int index = indexOfVideoFormat(format);
        if (index == -1)
            continue;
        GValue item = {};

        g_value_init(&item, G_TYPE_STRING);
        g_value_set_string(&item,
                           gst_video_format_to_string(qt_videoFormatLookup[index].gstFormat));
        gst_value_list_append_value(&list, &item);
        g_value_unset(&item);
    }

    auto *structure = gst_structure_new("video/x-raw", "framerate", GST_TYPE_FRACTION_RANGE, 0, 1,
                                        INT_MAX, 1, "width", GST_TYPE_INT_RANGE, 1, INT_MAX,
                                        "height", GST_TYPE_INT_RANGE, 1, INT_MAX, nullptr);
    gst_structure_set_value(structure, "format", &list);
    gst_caps_append_structure(get(), structure);
    g_value_unset(&list);

    if (modifier)
        gst_caps_set_features(get(), size() - 1, gst_caps_features_from_string(modifier));
}

void QGstCaps::setResolution(QSize resolution)
{
    Q_ASSERT(resolution.isValid());
    GValue width{};
    g_value_init(&width, G_TYPE_INT);
    g_value_set_int(&width, resolution.width());
    GValue height{};
    g_value_init(&height, G_TYPE_INT);
    g_value_set_int(&height, resolution.height());

    gst_caps_set_value(caps(), "width", &width);
    gst_caps_set_value(caps(), "height", &height);
}

QGstCaps QGstCaps::fromCameraFormat(const QCameraFormat &format)
{
    QSize size = format.resolution();
    GstStructure *structure = nullptr;
    if (format.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
        structure = gst_structure_new("image/jpeg", "width", G_TYPE_INT, size.width(), "height",
                                      G_TYPE_INT, size.height(), nullptr);
    } else {
        int index = indexOfVideoFormat(format.pixelFormat());
        if (index < 0)
            return {};
        auto gstFormat = qt_videoFormatLookup[index].gstFormat;
        structure = gst_structure_new("video/x-raw", "format", G_TYPE_STRING,
                                      gst_video_format_to_string(gstFormat), "width", G_TYPE_INT,
                                      size.width(), "height", G_TYPE_INT, size.height(), nullptr);
    }
    auto caps = QGstCaps::create();
    gst_caps_append_structure(caps.get(), structure);
    return caps;
}

QGstCaps::MemoryFormat QGstCaps::memoryFormat() const
{
    auto *features = gst_caps_get_features(get(), 0);
    if (gst_caps_features_contains(features, "memory:GLMemory"))
        return GLTexture;
    if (gst_caps_features_contains(features, "memory:DMABuf"))
        return DMABuf;
    return CpuMemory;
}

int QGstCaps::size() const
{
    return int(gst_caps_get_size(get()));
}

QGstStructure QGstCaps::at(int index) const
{
    return gst_caps_get_structure(get(), index);
}

GstCaps *QGstCaps::caps() const
{
    return get();
}

QGstCaps QGstCaps::create()
{
    return QGstCaps(gst_caps_new_empty(), HasRef);
}

// QGstObject

void QGstObject::set(const char *property, const char *str)
{
    g_object_set(get(), property, str, nullptr);
}

void QGstObject::set(const char *property, bool b)
{
    g_object_set(get(), property, gboolean(b), nullptr);
}

void QGstObject::set(const char *property, uint i)
{
    g_object_set(get(), property, guint(i), nullptr);
}

void QGstObject::set(const char *property, int i)
{
    g_object_set(get(), property, gint(i), nullptr);
}

void QGstObject::set(const char *property, qint64 i)
{
    g_object_set(get(), property, gint64(i), nullptr);
}

void QGstObject::set(const char *property, quint64 i)
{
    g_object_set(get(), property, guint64(i), nullptr);
}

void QGstObject::set(const char *property, double d)
{
    g_object_set(get(), property, gdouble(d), nullptr);
}

void QGstObject::set(const char *property, const QGstObject &o)
{
    g_object_set(get(), property, o.object(), nullptr);
}

void QGstObject::set(const char *property, const QGstCaps &c)
{
    g_object_set(get(), property, c.caps(), nullptr);
}

QGString QGstObject::getString(const char *property) const
{
    char *s = nullptr;
    g_object_get(get(), property, &s, nullptr);
    return QGString(s);
}

QGstStructure QGstObject::getStructure(const char *property) const
{
    GstStructure *s = nullptr;
    g_object_get(get(), property, &s, nullptr);
    return QGstStructure(s);
}

bool QGstObject::getBool(const char *property) const
{
    gboolean b = false;
    g_object_get(get(), property, &b, nullptr);
    return b;
}

uint QGstObject::getUInt(const char *property) const
{
    guint i = 0;
    g_object_get(get(), property, &i, nullptr);
    return i;
}

int QGstObject::getInt(const char *property) const
{
    gint i = 0;
    g_object_get(get(), property, &i, nullptr);
    return i;
}

quint64 QGstObject::getUInt64(const char *property) const
{
    guint64 i = 0;
    g_object_get(get(), property, &i, nullptr);
    return i;
}

qint64 QGstObject::getInt64(const char *property) const
{
    gint64 i = 0;
    g_object_get(get(), property, &i, nullptr);
    return i;
}

float QGstObject::getFloat(const char *property) const
{
    gfloat d = 0;
    g_object_get(get(), property, &d, nullptr);
    return d;
}

double QGstObject::getDouble(const char *property) const
{
    gdouble d = 0;
    g_object_get(get(), property, &d, nullptr);
    return d;
}

QGstObject QGstObject::getObject(const char *property) const
{
    GstObject *o = nullptr;
    g_object_get(get(), property, &o, nullptr);
    return QGstObject(o, HasRef);
}

QGObjectHandlerConnection QGstObject::connect(const char *name, GCallback callback,
                                              gpointer userData)
{
    return QGObjectHandlerConnection{
        *this,
        g_signal_connect(get(), name, callback, userData),
    };
}

void QGstObject::disconnect(gulong handlerId)
{
    g_signal_handler_disconnect(get(), handlerId);
}

GType QGstObject::type() const
{
    return G_OBJECT_TYPE(get());
}

GstObject *QGstObject::object() const
{
    return get();
}

const char *QGstObject::name() const
{
    return get() ? GST_OBJECT_NAME(get()) : "(null)";
}

// QGObjectHandlerConnection

QGObjectHandlerConnection::QGObjectHandlerConnection(QGstObject object, gulong handlerId)
    : object{ std::move(object) }, handlerId{ handlerId }
{
}

void QGObjectHandlerConnection::disconnect()
{
    if (!object)
        return;

    object.disconnect(handlerId);
    object = {};
    handlerId = invalidHandlerId;
}

// QGObjectHandlerScopedConnection

QGObjectHandlerScopedConnection::QGObjectHandlerScopedConnection(
        QGObjectHandlerConnection connection)
    : connection{
          std::move(connection),
      }
{
}

QGObjectHandlerScopedConnection::~QGObjectHandlerScopedConnection()
{
    connection.disconnect();
}

void QGObjectHandlerScopedConnection::disconnect()
{
    connection.disconnect();
}

// QGstPad

QGstPad::QGstPad(const QGstObject &o)
    : QGstPad{
          qGstSafeCast<GstPad>(o.object()),
          QGstElement::NeedsRef,
      }
{
}

QGstPad::QGstPad(GstPad *pad, RefMode mode)
    : QGstObject{
          qGstCheckedCast<GstObject>(pad),
          mode,
      }
{
}

QGstCaps QGstPad::currentCaps() const
{
    return QGstCaps(gst_pad_get_current_caps(pad()), QGstCaps::HasRef);
}

QGstCaps QGstPad::queryCaps() const
{
    return QGstCaps(gst_pad_query_caps(pad(), nullptr), QGstCaps::HasRef);
}

bool QGstPad::isLinked() const
{
    return gst_pad_is_linked(pad());
}

bool QGstPad::link(const QGstPad &sink) const
{
    return gst_pad_link(pad(), sink.pad()) == GST_PAD_LINK_OK;
}

bool QGstPad::unlink(const QGstPad &sink) const
{
    return gst_pad_unlink(pad(), sink.pad());
}

bool QGstPad::unlinkPeer() const
{
    return unlink(peer());
}

QGstPad QGstPad::peer() const
{
    return QGstPad(gst_pad_get_peer(pad()), HasRef);
}

QGstElement QGstPad::parent() const
{
    return QGstElement(gst_pad_get_parent_element(pad()), HasRef);
}

GstPad *QGstPad::pad() const
{
    return qGstCheckedCast<GstPad>(object());
}

GstEvent *QGstPad::stickyEvent(GstEventType type)
{
    return gst_pad_get_sticky_event(pad(), type, 0);
}

bool QGstPad::sendEvent(GstEvent *event)
{
    return gst_pad_send_event(pad(), event);
}

// QGstClock

QGstClock::QGstClock(const QGstObject &o)
    : QGstClock{
          qGstSafeCast<GstClock>(o.object()),
          QGstElement::NeedsRef,
      }
{
}

QGstClock::QGstClock(GstClock *clock, RefMode mode)
    : QGstObject{
          qGstCheckedCast<GstObject>(clock),
          mode,
      }
{
}

GstClock *QGstClock::clock() const
{
    return qGstCheckedCast<GstClock>(object());
}

GstClockTime QGstClock::time() const
{
    return gst_clock_get_time(clock());
}

// QGstElement

QGstElement::QGstElement(GstElement *element, RefMode mode)
    : QGstObject{
          qGstCheckedCast<GstObject>(element),
          mode,
      }
{
}

QGstElement QGstElement::createFromFactory(const char *factory, const char *name)
{
    GstElement *element = gst_element_factory_make(factory, name);

#ifndef QT_NO_DEBUG
    if (!element) {
        qWarning() << "Failed to make element" << name << "from factory" << factory;
        return QGstElement{};
    }
#endif

    return QGstElement{
        element,
        NeedsRef,
    };
}

QGstElement QGstElement::createFromDevice(const QGstDeviceHandle &device, const char *name)
{
    return createFromDevice(device.get(), name);
}

QGstElement QGstElement::createFromDevice(GstDevice *device, const char *name)
{
    return QGstElement{
        gst_device_create_element(device, name),
        QGstElement::NeedsRef,
    };
}

QGstPad QGstElement::staticPad(const char *name) const
{
    return QGstPad(gst_element_get_static_pad(element(), name), HasRef);
}

QGstPad QGstElement::src() const
{
    return staticPad("src");
}

QGstPad QGstElement::sink() const
{
    return staticPad("sink");
}

QGstPad QGstElement::getRequestPad(const char *name) const
{
#if GST_CHECK_VERSION(1, 19, 1)
    return QGstPad(gst_element_request_pad_simple(element(), name), HasRef);
#else
    return QGstPad(gst_element_get_request_pad(element(), name), HasRef);
#endif
}

void QGstElement::releaseRequestPad(const QGstPad &pad) const
{
    return gst_element_release_request_pad(element(), pad.pad());
}

GstState QGstElement::state(std::chrono::nanoseconds timeout) const
{
    using namespace std::chrono_literals;

    GstState state;
    GstStateChangeReturn change =
            gst_element_get_state(element(), &state, nullptr, timeout.count());

    if (Q_UNLIKELY(change == GST_STATE_CHANGE_ASYNC))
        qWarning() << "QGstElement::state detected an asynchronous state change. Return value not "
                      "reliable";

    return state;
}

GstStateChangeReturn QGstElement::setState(GstState state)
{
    return gst_element_set_state(element(), state);
}

bool QGstElement::setStateSync(GstState state, std::chrono::nanoseconds timeout)
{
    GstStateChangeReturn change = gst_element_set_state(element(), state);
    if (change == GST_STATE_CHANGE_ASYNC) {
        change = gst_element_get_state(element(), nullptr, &state, timeout.count());
    }
#ifndef QT_NO_DEBUG
    if (change != GST_STATE_CHANGE_SUCCESS && change != GST_STATE_CHANGE_NO_PREROLL)
        qWarning() << "Could not change state of" << name() << "to" << state << change;
#endif
    return change == GST_STATE_CHANGE_SUCCESS;
}

bool QGstElement::syncStateWithParent()
{
    Q_ASSERT(element());
    return gst_element_sync_state_with_parent(element()) == TRUE;
}

bool QGstElement::finishStateChange(std::chrono::nanoseconds timeout)
{
    GstState state, pending;
    GstStateChangeReturn change =
            gst_element_get_state(element(), &state, &pending, timeout.count());

#ifndef QT_NO_DEBUG
    if (change != GST_STATE_CHANGE_SUCCESS && change != GST_STATE_CHANGE_NO_PREROLL)
        qWarning() << "Could not finish change state of" << name() << change << state << pending;
#endif
    return change == GST_STATE_CHANGE_SUCCESS;
}

void QGstElement::lockState(bool locked)
{
    gst_element_set_locked_state(element(), locked);
}

bool QGstElement::isStateLocked() const
{
    return gst_element_is_locked_state(element());
}

void QGstElement::sendEvent(GstEvent *event) const
{
    gst_element_send_event(element(), event);
}

void QGstElement::sendEos() const
{
    sendEvent(gst_event_new_eos());
}

GstClockTime QGstElement::baseTime() const
{
    return gst_element_get_base_time(element());
}

void QGstElement::setBaseTime(GstClockTime time) const
{
    gst_element_set_base_time(element(), time);
}

GstElement *QGstElement::element() const
{
    return GST_ELEMENT_CAST(get());
}

QGstElement QGstElement::getParent() const
{
    return QGstElement{
        qGstCheckedCast<GstElement>(gst_element_get_parent(object())),
        QGstElement::HasRef,
    };
}

QGstPipeline QGstElement::getPipeline() const
{
    QGstElement ancestor = *this;
    for (;;) {
        QGstElement greatAncestor = ancestor.getParent();
        if (greatAncestor) {
            ancestor = std::move(greatAncestor);
            continue;
        }

        return QGstPipeline{
            qGstSafeCast<GstPipeline>(ancestor.element()),
            QGstPipeline::NeedsRef,
        };
    }
}

// QGstBin

QGstBin QGstBin::create(const char *name)
{
    return QGstBin(gst_bin_new(name), NeedsRef);
}

QGstBin QGstBin::createFromFactory(const char *factory, const char *name)
{
    QGstElement element = QGstElement::createFromFactory(factory, name);
    Q_ASSERT(GST_IS_BIN(element.element()));
    return QGstBin{
        GST_BIN(element.release()),
        RefMode::HasRef,
    };
}

QGstBin::QGstBin(GstBin *bin, RefMode mode)
    : QGstElement{
          qGstCheckedCast<GstElement>(bin),
          mode,
      }
{
}

GstBin *QGstBin::bin() const
{
    return qGstCheckedCast<GstBin>(object());
}

void QGstBin::addGhostPad(const QGstElement &child, const char *name)
{
    addGhostPad(name, child.staticPad(name));
}

void QGstBin::addGhostPad(const char *name, const QGstPad &pad)
{
    gst_element_add_pad(element(), gst_ghost_pad_new(name, pad.pad()));
}

bool QGstBin::syncChildrenState()
{
    return gst_bin_sync_children_states(bin());
}

void QGstBin::dumpGraph(const char *fileNamePrefix)
{
    if (isNull())
        return;

    GST_DEBUG_BIN_TO_DOT_FILE(bin(),
                              GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_ALL
                                                   | GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE
                                                   | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS
                                                   | GST_DEBUG_GRAPH_SHOW_STATES),
                              fileNamePrefix);
}

// QGstBaseSink

QGstBaseSink::QGstBaseSink(GstBaseSink *element, RefMode mode)
    : QGstElement{
          qGstCheckedCast<GstElement>(element),
          mode,
      }
{
}

GstBaseSink *QGstBaseSink::baseSink() const
{
    return qGstCheckedCast<GstBaseSink>(element());
}

// QGstBaseSrc

QGstBaseSrc::QGstBaseSrc(GstBaseSrc *element, RefMode mode)
    : QGstElement{
          qGstCheckedCast<GstElement>(element),
          mode,
      }
{
}

GstBaseSrc *QGstBaseSrc::baseSrc() const
{
    return qGstCheckedCast<GstBaseSrc>(element());
}

#if QT_CONFIG(gstreamer_app)

// QGstAppSink

QGstAppSink::QGstAppSink(GstAppSink *element, RefMode mode)
    : QGstBaseSink{
          qGstCheckedCast<GstBaseSink>(element),
          mode,
      }
{
}

QGstAppSink QGstAppSink::create(const char *name)
{
    QGstElement created = QGstElement::createFromFactory("appsink", name);
    return QGstAppSink{
        qGstCheckedCast<GstAppSink>(created.element()),
        QGstAppSink::NeedsRef,
    };
}

GstAppSink *QGstAppSink::appSink() const
{
    return qGstCheckedCast<GstAppSink>(element());
}

void QGstAppSink::setCaps(const QGstCaps &caps)
{
    gst_app_sink_set_caps(appSink(), caps.caps());
}

void QGstAppSink::setCallbacks(GstAppSinkCallbacks &callbacks, gpointer user_data,
                               GDestroyNotify notify)
{
    gst_app_sink_set_callbacks(appSink(), &callbacks, user_data, notify);
}

QGstSampleHandle QGstAppSink::pullSample()
{
    return QGstSampleHandle{
        gst_app_sink_pull_sample(appSink()),
        QGstSampleHandle::HasRef,
    };
}

// QGstAppSrc

QGstAppSrc::QGstAppSrc(GstAppSrc *element, RefMode mode)
    : QGstBaseSrc{
          qGstCheckedCast<GstBaseSrc>(element),
          mode,
      }
{
}

QGstAppSrc QGstAppSrc::create(const char *name)
{
    QGstElement created = QGstElement::createFromFactory("appsrc", name);
    return QGstAppSrc{
        qGstCheckedCast<GstAppSrc>(created.element()),
        QGstAppSrc::NeedsRef,
    };
}

GstAppSrc *QGstAppSrc::appSrc() const
{
    return qGstCheckedCast<GstAppSrc>(element());
}

void QGstAppSrc::setCallbacks(GstAppSrcCallbacks &callbacks, gpointer user_data,
                              GDestroyNotify notify)
{
    gst_app_src_set_callbacks(appSrc(), &callbacks, user_data, notify);
}

GstFlowReturn QGstAppSrc::pushBuffer(GstBuffer *buffer)
{
    return gst_app_src_push_buffer(appSrc(), buffer);
}

#endif

QT_END_NAMESPACE
