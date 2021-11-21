#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Logic.h"
#include <QObject>
#include <QVector2D>

class SideViewWidget;
class TopViewWidget;
class CentralWidget;
class AxisWidget;

namespace Dori {
namespace Core {
class Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller(QObject *parent = nullptr);

    struct Camera
    {
        float height;
        float tiltAngle;
        QPointF position;
    };

    struct Target
    {
        float height;
        float distance;
        QPointF position;
    };

    struct LowerBoundary : Target
    {};

    struct SideViewWidgetParameters
    {
        Camera camera;
        Target target;
        LowerBoundary lowerBoundary;

        // Ground intersections
        QPointF points[4]; // OPPOSITE_BISECTOR, BISECTOR, V1, V2
    };

    struct TopViewWidgetParamaters
    {
        float targetDistance;
        float fovWidth;

        // Intersections
        QPointF ground[4];
        QPointF target[4];
        QPointF lowerBoundary[4];
    };

    CentralWidget *centralWidget();

public slots:
    void onDirty();
    void onZoom(int);
    void onPan(float x, float y);
    void init();

private:
    void calculate();
    void update();

    void setMeterToPixelRatio(float newMeterToPixelRatio);
    void setOrigin(QPointF newOrigin);

    Dori::Core::Logic &mLogic;

    Dori::Core::Logic::Parameters *mLogicParameters;
    SideViewWidgetParameters *mSideViewWidgetParameters;
    TopViewWidgetParamaters *mTopViewWidgetParameters;

    SideViewWidget *mSideViewWidget;
    TopViewWidget *mTopViewWidget;
    CentralWidget *mCentralWidget;
    AxisWidget *mAxisWidget;

    const float mZoomStepSize;

    float mMeterToPixelRatio;
    QPointF mOrigin;
};

};     // namespace Core
};     // namespace Dori
#endif // CONTROLLER_H
