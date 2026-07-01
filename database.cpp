#include "database.h"
#include <QCoreApplication>
#include <QDir>

Database& Database::instance()
{
    static Database inst;
    return inst;
}

Database::~Database()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Database::init()
{
    // 数据库文件放在程序运行目录下
    QString dbPath = QCoreApplication::applicationDirPath() + "/annotation.db";
    qDebug() << "数据库路径:" << dbPath;

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "数据库打开失败:" << m_db.lastError().text();
        return false;
    }

    createTables();
    qDebug() << "数据库初始化成功";
    return true;
}

void Database::createTables()
{
    QSqlQuery query;

    // 训练记录表
    QString createTrainTable =
        "CREATE TABLE IF NOT EXISTS train_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "model_path TEXT,"
        "dataset TEXT,"
        "epochs INTEGER,"
        "batch_size INTEGER,"
        "learning_rate REAL,"
        "map50 REAL,"
        "status TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")";

    if (!query.exec(createTrainTable)) {
        qDebug() << "创建训练记录表失败:" << query.lastError().text();
    }

    // 推理记录表
    QString createInferTable =
        "CREATE TABLE IF NOT EXISTS infer_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "model_path TEXT,"
        "image_path TEXT,"
        "result_text TEXT,"
        "result_image_path TEXT,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")";

    if (!query.exec(createInferTable)) {
        qDebug() << "创建推理记录表失败:" << query.lastError().text();
    }
}

bool Database::saveTrainRecord(const QString& modelPath,
                               const QString& dataset,
                               int epochs,
                               int batchSize,
                               double learningRate,
                               double mAP50,
                               const QString& status)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO train_records "
        "(model_path, dataset, epochs, batch_size, learning_rate, map50, status) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"
        );
    query.addBindValue(modelPath);
    query.addBindValue(dataset);
    query.addBindValue(epochs);
    query.addBindValue(batchSize);
    query.addBindValue(learningRate);
    query.addBindValue(mAP50);
    query.addBindValue(status);

    if (!query.exec()) {
        qDebug() << "保存训练记录失败:" << query.lastError().text();
        return false;
    }
    qDebug() << "训练记录已保存";
    return true;
}

bool Database::saveInferRecord(const QString& modelPath,
                               const QString& imagePath,
                               const QString& resultText,
                               const QString& resultImagePath)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO infer_records "
        "(model_path, image_path, result_text, result_image_path) "
        "VALUES (?, ?, ?, ?)"
        );
    query.addBindValue(modelPath);
    query.addBindValue(imagePath);
    query.addBindValue(resultText);
    query.addBindValue(resultImagePath);

    if (!query.exec()) {
        qDebug() << "保存推理记录失败:" << query.lastError().text();
        return false;
    }
    qDebug() << "推理记录已保存";
    return true;
}