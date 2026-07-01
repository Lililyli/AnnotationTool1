#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariantList>
#include <QDateTime>
#include <QDebug>

// 单例模式 - 整个程序只有一个数据库实例
class Database
{
public:
    // 获取唯一实例
    static Database& instance();

    // 初始化数据库（程序启动时调用）
    bool init();

    // ===== 训练记录 =====
    bool saveTrainRecord(const QString& modelPath,
                         const QString& dataset,
                         int epochs,
                         int batchSize,
                         double learningRate,
                         double mAP50,
                         const QString& status);

    // ===== 推理记录 =====
    bool saveInferRecord(const QString& modelPath,
                         const QString& imagePath,
                         const QString& resultText,
                         const QString& resultImagePath);

private:
    Database() {}  // 私有构造函数
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void createTables();

    QSqlDatabase m_db;
};

#endif // DATABASE_H