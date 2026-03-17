#include "camera_controller.h"
#include <QtMath>

CameraController::CameraController()
    : m_targetRotX(0), m_targetRotY(0)
    , m_currentRotX(0), m_currentRotY(0)
    , m_aspect(4.0f)
{}

void CameraController::setViewport(int w, int h) {
    m_aspect = (h > 0) ? (float)w / h : 4.0f;
}

void CameraController::setMousePosition(float nx, float ny) {
    m_targetRotY = (nx - 0.5f) * 2.0f * maxAngle;
    m_targetRotX = (ny - 0.5f) * 2.0f * maxAngle;
}

void CameraController::update() {
    m_currentRotX += (m_targetRotX - m_currentRotX) * lerpSpeed;
    m_currentRotY += (m_targetRotY - m_currentRotY) * lerpSpeed;
}

QMatrix4x4 CameraController::viewMatrix() const {
    QMatrix4x4 view;
    view.translate(0, 0, -1);
    view.rotate(m_currentRotX, 1, 0, 0);
    view.rotate(m_currentRotY, 0, 1, 0);
    return view;
}

QMatrix4x4 CameraController::projectionMatrix() const {
    QMatrix4x4 proj;
    proj.ortho(-m_aspect, m_aspect, -1.0f, 1.0f, 0.1f, 100.0f);
    return proj;
}
