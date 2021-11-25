#include "Controller.h"

#include <Core/Enums.h>
#include <GUI/Widgets/CentralWidget.h>
#include <GUI/Widgets/SideViewWidget.h>
#include <GUI/Widgets/TopViewWidget.h>

#include <QDebug>
#include <QtMath>

Controller::Controller(QObject *parent)
    : QObject(parent)
    , mLogic(Logic::getInstance())
    , mZoomStepSize(2.0f)
    , mOrigin(128, 368)
{}

CentralWidget *Controller::centralWidget()
{
    return mCentralWidget;
}

void Controller::calculate()
{
    mLogic.calculate();

    // SideViewWidgetParameters
    {
        mSideViewWidgetParameters->camera.tiltAngle = mLogicParameters->camera.tiltAngle;
        mSideViewWidgetParameters->camera.height = mLogicParameters->camera.height;
        mSideViewWidgetParameters->camera.position = mSideViewWidget->mapFrom3d(0, mLogicParameters->camera.height);
        mSideViewWidgetParameters->target.height = mLogicParameters->target.height;
        mSideViewWidgetParameters->target.distance = mLogicParameters->target.distance;
        mSideViewWidgetParameters->target.position = mSideViewWidget->mapFrom3d(mLogicParameters->target.distance, mLogicParameters->target.height);
        mSideViewWidgetParameters->lowerBoundary.height = mLogicParameters->lowerBoundary.height;
        mSideViewWidgetParameters->lowerBoundary.distance = mLogicParameters->lowerBoundary.distance;
        mSideViewWidgetParameters->lowerBoundary.position = mSideViewWidget->mapFrom3d(mLogicParameters->lowerBoundary.distance, mLogicParameters->lowerBoundary.height);

        mSideViewWidgetParameters->points[0] = mSideViewWidget->mapFrom3d(mLogicParameters->frustum.oppositeBisectorRay);
        mSideViewWidgetParameters->points[1] = mSideViewWidget->mapFrom3d(mLogicParameters->frustum.bisectorRay);
        mSideViewWidgetParameters->points[2] = mSideViewWidget->mapFrom3d(mLogicParameters->frustum.bottomVertices[0]);
        mSideViewWidgetParameters->points[3] = mSideViewWidget->mapFrom3d(mLogicParameters->frustum.bottomVertices[1]);

        QPolygonF roi;
        roi.append(mSideViewWidget->mapFrom3d(mLogicParameters->target.distance, mLogicParameters->target.height));
        roi.append(mSideViewWidget->mapFrom3d(mLogicParameters->target.distance, mLogicParameters->lowerBoundary.height));
        roi.append(mSideViewWidget->mapFrom3d(mLogicParameters->lowerBoundary.distance, mLogicParameters->lowerBoundary.height));
        roi.append(mSideViewWidget->mapFrom3d(mLogicParameters->lowerBoundary.distance, mLogicParameters->target.height));

        for (int i = 0; i < NUMBER_OF_REGIONS; ++i) {
            QPolygonF region;

            region.append(mSideViewWidget->mapFrom3d(mLogicParameters->regions[i].bottomVertices[0]));
            region.append(mSideViewWidget->mapFrom3d(mLogicParameters->regions[i].topVertices[0]));
            region.append(mSideViewWidget->mapFrom3d(mLogicParameters->regions[i].topVertices[2]));
            region.append(mSideViewWidget->mapFrom3d(mLogicParameters->regions[i].bottomVertices[2]));

            region = region.intersected(roi);
            mSideViewWidgetParameters->regions[i].region = region;
            mSideViewWidgetParameters->regions[i].visible = !region.isEmpty();
        }
    }

    // TopViewWidgetParameters
    {
        mTopViewWidgetParameters->targetDistance = mLogicParameters->target.distance;
        mTopViewWidgetParameters->fovWidth = mLogicParameters->target.fovWidth;

        for (int i = 0; i < 4; ++i) {
            mTopViewWidgetParameters->ground[i] = mTopViewWidget->mapFrom3d(mLogicParameters->frustum.bottomVertices[i]);
            mTopViewWidgetParameters->target[i] = mTopViewWidget->mapFrom3d(mLogicParameters->target.intersections[i]);
            mTopViewWidgetParameters->lowerBoundary[i] = mTopViewWidget->mapFrom3d(mLogicParameters->lowerBoundary.intersections[i]);
        }

        QPolygonF targetRoi;

        if (mLogicParameters->camera.height <= mLogicParameters->target.height) {
            targetRoi.append(mTopViewWidget->mapFrom3d(0, 0));
            targetRoi.append(mTopViewWidgetParameters->target[0]);
            targetRoi.append(mTopViewWidgetParameters->target[3]);
        } else {
            targetRoi.append(mTopViewWidgetParameters->target[0]);
            targetRoi.append(mTopViewWidgetParameters->target[1]);
            targetRoi.append(mTopViewWidgetParameters->target[2]);
            targetRoi.append(mTopViewWidgetParameters->target[3]);
        }

        QPolygonF lowerBoundaryRoi;
        lowerBoundaryRoi.append(mTopViewWidgetParameters->lowerBoundary[0]);
        lowerBoundaryRoi.append(mTopViewWidgetParameters->lowerBoundary[1]);
        lowerBoundaryRoi.append(mTopViewWidgetParameters->lowerBoundary[2]);
        lowerBoundaryRoi.append(mTopViewWidgetParameters->lowerBoundary[3]);

        QPolygonF roi = lowerBoundaryRoi.intersected(targetRoi);

        for (int i = 0; i < NUMBER_OF_REGIONS; ++i) {
            Eigen::Hyperplane<float, 3> plane(Eigen::Vector3f(0, 0, 1), -mLogicParameters->lowerBoundary.height);
            QVector<Eigen::Vector3f> intersections = mLogic.findIntersection(mLogicParameters->regions[i], plane);

            QPolygonF region;
            for (int i = 0; i < intersections.size(); ++i) {
                region.append(mTopViewWidget->mapFrom3d(intersections[i]));
            }

            region = region.intersected(roi);
            mTopViewWidgetParameters->regions[i].region = region;
            mTopViewWidgetParameters->regions[i].visible = !region.isEmpty();
        }
    }
}

void Controller::update()
{
    calculate();
    mSideViewWidget->refresh();
    mTopViewWidget->refresh();
}

void Controller::onDirty()
{
    QObject *sender = QObject::sender();
    if (sender == mSideViewWidget) {
        mLogicParameters->camera.height = mSideViewWidgetParameters->camera.height;
        mLogicParameters->target.height = mSideViewWidgetParameters->target.height;
        mLogicParameters->target.distance = mSideViewWidgetParameters->target.distance;
        mLogicParameters->lowerBoundary.height = mSideViewWidgetParameters->lowerBoundary.height;
    } else if (sender == mTopViewWidget) {
        mLogicParameters->target.distance = mTopViewWidgetParameters->targetDistance;
        mLogicParameters->target.fovWidth = mTopViewWidgetParameters->fovWidth;
    }

    update();
}

void Controller::onZoom(int i)
{
    if (i < 0) {
        setMeterToPixelRatio(mZoomStepSize * mMeterToPixelRatio);
    } else if (i > 0) {
        setMeterToPixelRatio(mMeterToPixelRatio / mZoomStepSize);
    }
}

void Controller::onPan(int x, int y)
{
    setOrigin(QPointF(mOrigin.x() + x, mOrigin.y() + y));
}

void Controller::init()
{
    mLogicParameters = new Logic::Parameters;
    mLogicParameters->camera.height = 5;
    mLogicParameters->target.height = 2;
    mLogicParameters->target.distance = 5;
    mLogicParameters->lowerBoundary.height = 0;
    mLogicParameters->lowerBoundary.distance = 0;

    mLogicParameters->frustum.zNear = 0.0001;
    mLogicParameters->frustum.zFar = 1000;

    mLogicParameters->camera.sensor.width = 1920.0f;
    mLogicParameters->camera.sensor.height = 1080.0f;
    mLogicParameters->camera.sensor.aspectRatio = 1920.0f / 1080.0f;
    mLogicParameters->target.fovWidth = 10;
    mLogicParameters->frustum.horizontalFov = mLogic.calculateHorizontalFovForGivenFovWidth(mLogicParameters->target, mLogicParameters->camera);

    mLogic.setParameters(mLogicParameters);

    mCentralWidget = new CentralWidget;
    mCentralWidget->init();

    mSideViewWidget = mCentralWidget->sideViewWidget();
    mSideViewWidgetParameters = new SideViewWidgetParameters;
    mSideViewWidget->setParameters(mSideViewWidgetParameters);

    mTopViewWidget = mCentralWidget->topViewWidget();
    mTopViewWidgetParameters = new TopViewWidgetParamaters;
    mTopViewWidget->setParameters(mTopViewWidgetParameters);

    mAxisWidget = mCentralWidget->axisWidget();

    // Connections
    connect(mSideViewWidget, &SideViewWidget::dirty, this, &Controller::onDirty);
    connect(mSideViewWidget, &SideViewWidget::zoom, this, &Controller::onZoom);
    connect(mSideViewWidget, &SideViewWidget::pan, this, &Controller::onPan);

    connect(mTopViewWidget, &TopViewWidget::dirty, this, &Controller::onDirty);
    connect(mTopViewWidget, &TopViewWidget::zoom, this, &Controller::onZoom);
    connect(mTopViewWidget, &TopViewWidget::pan, this, &Controller::onPan);

    setMeterToPixelRatio(10);
    setOrigin(mOrigin);

    update();
}

void Controller::setMeterToPixelRatio(float newMeterToPixelRatio)
{
    if (newMeterToPixelRatio < 4.0 || newMeterToPixelRatio > 128.0f) {
        return;
    }

    mMeterToPixelRatio = newMeterToPixelRatio;

    mSideViewWidget->setMeterToPixelRatio(newMeterToPixelRatio);
    mTopViewWidget->setMeterToPixelRatio(newMeterToPixelRatio);
    mAxisWidget->setMeterToPixelRatio(newMeterToPixelRatio);
    mAxisWidget->refresh();

    update();
}

void Controller::setOrigin(QPointF newOrigin)
{
    mOrigin = newOrigin;

    mSideViewWidget->setOrigin(newOrigin);
    mAxisWidget->setOrigin(newOrigin);
    mTopViewWidget->setOrigin(newOrigin);

    mAxisWidget->refresh();
    update();
}
