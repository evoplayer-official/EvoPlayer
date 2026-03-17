#pragma once
#include <GL/glew.h>
#include <QString>
#include <QVector>
#include <QVector3D>

struct MeshData {
    QString name;
    QVector<float> vertices;
    QVector<unsigned int> indices;
    GLuint VAO = 0, VBO = 0, EBO = 0;
    QVector3D baseColor;
    QVector3D emissiveColor;
    bool isEmissive = false;
};

class ModelLoader {
public:
    ModelLoader();
    ~ModelLoader();
    bool load(const QString &path);
    void uploadToGPU();
    const QVector<MeshData>& meshes() const { return m_meshes; }

private:
    QVector<MeshData> m_meshes;
    void processNode(void* node, void* scene);
    MeshData processMesh(void* mesh, void* scene);
};
