#ifndef IMAGEGRABBER_H
#define IMAGEGRABBER_H

#include <QList>
#include <QPixmap>
#include <QScreen>
#include <QDesktopWidget>
#include <QApplication>

class ImageGrabber : public QObject
{
    Q_OBJECT
    Q_ENUMS(GrabMode)

    Q_PROPERTY(QPixmap pixmap READ pixmap NOTIFY pixmapChanged)
    Q_PROPERTY(bool capturePointer READ capturePointer WRITE setCapturePointer NOTIFY capturePointerChanged)
    Q_PROPERTY(bool captureDecorations READ captureDecorations WRITE setCaptureDecorations NOTIFY captureDecorationsChanged)
    Q_PROPERTY(GrabMode grabMode READ grabMode WRITE setGrabMode NOTIFY grabModeChanged)

    public:

    enum GrabMode {
        InvalidChoice     = -1,
        FullScreen        = 0,
        CurrentScreen     = 1,
        ActiveWindow      = 2,
        RectangularRegion = 3
    };

    explicit ImageGrabber(QObject *parent = 0);
    ~ImageGrabber();

    QPixmap pixmap() const;
    bool capturePointer() const;
    bool captureDecorations() const;
    GrabMode grabMode() const;

    void setCapturePointer(const bool newCapturePointer);
    void setCaptureDecorations(const bool newCaptureDecorations);
    void setGrabMode(const GrabMode newGrabMode);

    signals:

    void pixmapChanged(const QPixmap pixmap);
    void imageGrabFailed();
    void capturePointerChanged(bool capturePointer);
    void captureDecorationsChanged(bool captureDecorations);
    void grabModeChanged(GrabMode grabMode);

    public slots:

    void doImageGrab();

    protected:

    virtual void grabFullScreen();
    virtual void grabCurrentScreen();
    virtual void grabActiveWindow();
    virtual void blendCursorImage(int x, int y, int width, int height) = 0;

    bool     mCapturePointer;
    bool     mCaptureDecorations;
    GrabMode mGrabMode;
    QPixmap  mPixmap;
};

#endif // IMAGEGRABBER_H
