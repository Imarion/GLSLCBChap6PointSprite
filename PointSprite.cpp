#include "PointSprite.h"

#include <QtGlobal>

#include <QDebug>
#include <QFile>
#include <QImage>
#include <QTime>

#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>

#include <cmath>
#include <ctime>
#include <cstring>
#include <cstdlib>

MyWindow::~MyWindow()
{
    if (mProgram  != 0)   delete   mProgram;
}

MyWindow::MyWindow() : currentTimeMs(0), currentTimeS(0), numSprites(50)
{
    mProgram  = 0;

    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    create();

    resize(800, 600);

    mContext = new QOpenGLContext(this);
    mContext->setFormat(format);
    mContext->create();

    mContext->makeCurrent( this );

    mFuncs = mContext->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if ( !mFuncs )
    {
        qWarning( "Could not obtain OpenGL versions object" );
        exit( 1 );
    }
    if (mFuncs->initializeOpenGLFunctions() == GL_FALSE)
    {
        qWarning( "Could not initialize core open GL functions" );
        exit( 1 );
    }

    initializeOpenGLFunctions();

    QTimer *repaintTimer = new QTimer(this);
    connect(repaintTimer, &QTimer::timeout, this, &MyWindow::render);
    repaintTimer->start(1000/60);

    QTimer *elapsedTimer = new QTimer(this);
    connect(elapsedTimer, &QTimer::timeout, this, &MyWindow::modCurTime);
    elapsedTimer->start(1);       
}

void MyWindow::modCurTime()
{
    currentTimeMs++;
    currentTimeS=currentTimeMs/1000.0f;
}

void MyWindow::initialize()
{
    mFuncs->glGenVertexArrays(1, &mVAO);
    mFuncs->glBindVertexArray(mVAO);

    CreateVertexBuffer();
    initShaders();

    mModelViewMatrixLocation = mProgram->uniformLocation("ModelViewMatrix");
    mProjMatrixLocation      = mProgram->uniformLocation("ProjectionMatrix");

    PrepareTexture(GL_TEXTURE0, GL_TEXTURE_2D, "../Media/flower.png", true);

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void MyWindow::CreateVertexBuffer()
{

    float *locations;
    locations = new float[numSprites * 3];
    srand( (unsigned int)time(0) );
    for( int i = 0; i < numSprites; i++ ) {
        QVector3D p(((float)rand() / RAND_MAX * 2.0f) - 1.0f,
                    ((float)rand() / RAND_MAX * 2.0f) - 1.0f,
                    ((float)rand() / RAND_MAX * 2.0f) - 1.0f);
        locations[i*3]   = p.x();
        locations[i*3+1] = p.y();
        locations[i*3+2] = p.z();
    }

    // Set up the buffers
    GLuint handle;
    glGenBuffers(1, &handle);

    glBindBuffer(GL_ARRAY_BUFFER, handle);
    glBufferData(GL_ARRAY_BUFFER, numSprites * 3 * sizeof(float), locations, GL_STATIC_DRAW);

    delete[] locations;

    // Set up the vertex array object
    mFuncs->glGenVertexArrays(1, &sprites);
    mFuncs->glBindVertexArray(sprites);

    glBindBuffer(GL_ARRAY_BUFFER, handle);
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, ((GLubyte *)NULL + (0)) );
    glEnableVertexAttribArray(0);  // Vertex position

    mFuncs->glBindVertexArray(0);
}

void MyWindow::resizeEvent(QResizeEvent *)
{
    mUpdateSize = true;
    ProjMatrix.setToIdentity();
    ProjMatrix.perspective(60.0f, (float)this->width()/(float)this->height(), 0.3f, 100.0f);
}

void MyWindow::render()
{
    if(!isVisible() || !isExposed())
        return;

    if (!mContext->makeCurrent(this))
        return;

    static bool initialized = false;
    if (!initialized) {
        initialize();
        initialized = true;
    }

    if (mUpdateSize) {
        glViewport(0, 0, size().width(), size().height());
        mUpdateSize = false;
    }

    static float EvolvingVal = 0;
    EvolvingVal += 0.1f;

    QMatrix4x4 MVMatrix;

    QVector3D cameraPos(0.0f, 0.0f, 3.0f);
    MVMatrix.lookAt(cameraPos, QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f));

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        

    mFuncs->glBindVertexArray(sprites);

    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    mProgram->bind();
    {
        glUniformMatrix4fv(mModelViewMatrixLocation, 1, GL_FALSE, MVMatrix.constData());
        glUniformMatrix4fv(mProjMatrixLocation, 1, GL_FALSE, ProjMatrix.constData());

        glDrawArrays(GL_POINTS, 0, numSprites);

        glDisableVertexAttribArray(0);        
    }
    mProgram->release();

    mContext->swapBuffers(this);
}

void MyWindow::initShaders()
{
    QOpenGLShader vShader(QOpenGLShader::Vertex);
    QOpenGLShader gShader(QOpenGLShader::Geometry);
    QOpenGLShader fShader(QOpenGLShader::Fragment);
    QFile         shaderFile;
    QByteArray    shaderSource;

    shaderFile.setFileName(":/vshader.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vertex   compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/gshader.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "geometry compile: " << gShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag     compile: " << fShader.compileSourceCode(shaderSource);

    mProgram = new (QOpenGLShaderProgram);
    mProgram->addShader(&vShader);
    mProgram->addShader(&gShader);
    mProgram->addShader(&fShader);
    qDebug() << "shader link tree: " << mProgram->link();
}

void MyWindow::PrepareTexture(GLenum TextureUnit, GLenum TextureTarget, const QString& FileName, bool flip)
{
    QImage TexImg;

    if (!TexImg.load(FileName)) qDebug() << "Erreur chargement texture " << FileName;
    if (flip==true) TexImg=TexImg.mirrored();

    glActiveTexture(TextureUnit);
    GLuint TexObject;
    glGenTextures(1, &TexObject);
    glBindTexture(TextureTarget, TexObject);
    mFuncs->glTexStorage2D(TextureTarget, 1, GL_RGB8, TexImg.width(), TexImg.height());
    mFuncs->glTexSubImage2D(TextureTarget, 0, 0, 0, TexImg.width(), TexImg.height(), GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    //glTexImage2D(TextureTarget, 0, GL_RGB, TexImg.width(), TexImg.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    glTexParameteri(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void MyWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch(keyEvent->key())
    {
        case Qt::Key_P:
            break;
        case Qt::Key_Up:
            break;
        case Qt::Key_Down:
            break;
        case Qt::Key_Left:
            break;
        case Qt::Key_Right:
            break;
        case Qt::Key_Delete:
            break;
        case Qt::Key_PageDown:
            break;
        case Qt::Key_Home:
            break;
        case Qt::Key_Z:
            break;
        case Qt::Key_Q:
            break;
        case Qt::Key_S:
            break;
        case Qt::Key_D:
            break;
        case Qt::Key_A:
            break;
        case Qt::Key_E:
            break;
        default:
            break;
    }
}


void MyWindow::printMatrix(const QMatrix4x4& mat)
{
    const float *locMat = mat.transposed().constData();

    for (int i=0; i<4; i++)
    {
        qDebug() << locMat[i*4] << " " << locMat[i*4+1] << " " << locMat[i*4+2] << " " << locMat[i*4+3];
    }
}
