#pragma once
#include <QMatrix4x4>

class CameraController {
public:
    CameraController();
    void setViewport(int w, int h);
    void setMousePosition(float nx, float ny);
    void update();
    QMatrix4x4 viewMatrix() const;
    QMatrix4x4 projectionMatrix() const;

private:
    float m_targetRotX, m_targetRotY;
    float m_currentRotX, m_currentRotY;
    float m_aspect;
    const float maxAngle = 2.5f;
    const float lerpSpeed = 0.05f;
};
