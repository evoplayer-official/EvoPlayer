#include <GL/glew.h>
#include "model_loader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <QDebug>

ModelLoader::ModelLoader() {}

ModelLoader::~ModelLoader() {
    for (auto &m : m_meshes) {
        if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
        if (m.VBO) glDeleteBuffers(1, &m.VBO);
        if (m.EBO) glDeleteBuffers(1, &m.EBO);
    }
}

bool ModelLoader::load(const QString &path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.toStdString(),
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs);
    if (!scene || !scene->mRootNode) {
        qWarning() << "ModelLoader: errore caricamento" << path;
        return false;
    }
    processNode(scene->mRootNode, (void*)scene);
    qDebug() << "ModelLoader: caricate" << m_meshes.size() << "mesh da" << path;
    return true;
}

void ModelLoader::processNode(void* nodePtr, void* scenePtr) {
    aiNode* node = (aiNode*)nodePtr;
    aiScene* scene = (aiScene*)scenePtr;
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.append(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

MeshData ModelLoader::processMesh(void* meshPtr, void* scenePtr) {
    aiMesh* mesh = (aiMesh*)meshPtr;
    aiScene* scene = (aiScene*)scenePtr;
    MeshData data;
    data.name = QString(mesh->mName.C_Str());

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        data.vertices.append(mesh->mVertices[i].x);
        data.vertices.append(mesh->mVertices[i].y);
        data.vertices.append(mesh->mVertices[i].z);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            data.indices.append(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        aiColor3D col(0,0,0), em(0,0,0);
        mat->Get(AI_MATKEY_COLOR_DIFFUSE, col);
        mat->Get(AI_MATKEY_COLOR_EMISSIVE, em);
        data.baseColor = QVector3D(col.r, col.g, col.b);
        data.emissiveColor = QVector3D(em.r, em.g, em.b);
        data.isEmissive = (em.r + em.g + em.b) > 0.1f;
    }
    return data;
}

void ModelLoader::uploadToGPU() {
    for (auto &m : m_meshes) {
        glGenVertexArrays(1, &m.VAO);
        glGenBuffers(1, &m.VBO);
        glGenBuffers(1, &m.EBO);

        glBindVertexArray(m.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
        glBufferData(GL_ARRAY_BUFFER,
            m.vertices.size() * sizeof(float),
            m.vertices.constData(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            m.indices.size() * sizeof(unsigned int),
            m.indices.constData(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
            3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    qDebug() << "ModelLoader: uploadToGPU completato";
}
