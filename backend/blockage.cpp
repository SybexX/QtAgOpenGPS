#include "blockage.h"
#include "agioservice.h"
#include "pgnparser.h"

Q_LOGGING_CATEGORY (blockage_log, "backend.qtagopengps")

Blockage *Blockage::s_instance = nullptr;
QMutex Blockage::s_mutex;
bool Blockage::s_cpp_created = false;

Blockage::Blockage(QObject *parent)
    : QObject{parent}
{
    //connect us to agio
    connect(AgIOService::instance(), &AgIOService::blockageDataReady,
            this, &Blockage::onBlockageDataReady, Qt::DirectConnection);

    connect(&updateTimeout, &QTimer::timeout, this, &Blockage::on_timeout);

    lastUpdate.restart(); //zero the update timer
    updateTimeout.setInterval(1000); //1 second timer
    updateTimeout.start();

    m_blockageModel = new BlockageModel(this);
}

Blockage *Blockage::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new Blockage();
        qDebug(blockage_log) << "Blockage singleton created by C++ code.";
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

Blockage *Blockage::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new Blockage();
        qDebug(blockage_log) << "Blockage singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

void Blockage::zeroDisplay() {
    m_blockageModel->zeroCounts();
}

void Blockage::startTimeout() {
    updateTimeout.start();
}

void Blockage::stopTimeout(){
    updateTimeout.stop();
}

void Blockage::on_timeout() {
    //if more than 5 seconds of no activity, zero out the display
    if (lastUpdate.elapsed() > m_timeout) {
        zeroDisplay();
        lastUpdate.restart();
    }
}

void Blockage::onBlockageDataReady(const PGNParser::ParsedData &data)
{

    // Update data from Blockage modules

    if (!data.isValid) return;

    // PGN 244: Blockage Data
    if (data.pgnNumber == 244) {

        static int iteration[4] = {0, 0, 0, 0};

        int i = data.blockagesection[1];
        int sectionType = data.blockagesection[0];
        int value = data.blockagesection[2];

        if (i >= 0) {
            switch(sectionType) {
            case 0:
                if (i < (int)(sizeof(blockageSecCount1) / sizeof(blockageSecCount1[0])))
                    blockageSecCount1[i] = value;
                iteration[sectionType] = 0;
                break;
            case 1:
                if (i < (int)(sizeof(blockageSecCount2) / sizeof(blockageSecCount2[0])))
                    blockageSecCount2[i] = value;
                iteration[sectionType] = 0;
                break;
            case 2:
                if (i < (int)(sizeof(blockageSecCount3) / sizeof(blockageSecCount3[0])))
                    blockageSecCount3[i] = value;
                iteration[sectionType] = 0;
                break;
            case 3:
                if (i < (int)(sizeof(blockageSecCount4) / sizeof(blockageSecCount4[0])))
                    blockageSecCount4[i] = value;
                iteration[sectionType] = 0;
                break;
            }
        }

        if(lastUpdate.elapsed() >= 1000) {
            //It's been longer than 1 second since the last update

            lastUpdate.restart(); //restart the count

            statistics();
            // обновляем данные каждую секунду

            for (int i = 0; i < 4; i++) {
                iteration[i]++;
            }
            // обнуляем если данные перестали поступать
            if (iteration[0] > 5) {
                memset(blockageSecCount1, 0, sizeof(blockageSecCount1));
                iteration[0] = 99;
            }
            if (iteration[1] > 5) {
                memset(blockageSecCount2, 0, sizeof(blockageSecCount2));
                iteration[1] = 99;
            }
            if (iteration[2] > 5) {
                memset(blockageSecCount3, 0, sizeof(blockageSecCount3));
                iteration[2] = 99;
            }
            if (iteration[3] > 5) {
                memset(blockageSecCount4, 0, sizeof(blockageSecCount4));
                iteration[3] = 99;
            }
        }
    }

}

void Blockage::statistics(){
    int k = 0;
    int numRows1 = SettingsManager::instance()->seed_blockRow1();
    int numRows2 = SettingsManager::instance()->seed_blockRow2();
    int numRows3 = SettingsManager::instance()->seed_blockRow3();
    int numRows4 = SettingsManager::instance()->seed_blockRow4();
    int numRows = SettingsManager::instance()->seed_numRows();
    int blockCountMin = SettingsManager::instance()->seed_blockCountMin();
    double toolWidth = SettingsManager::instance()->vehicle_toolWidth();
    double rowwidth = toolWidth / numRows;

    QVector<BlockageModel::Row> rowCount;
    rowCount.reserve(MAX_SECTIONS);
    for (int i = 0; i < (numRows1 + numRows2 + numRows3 + numRows4) ; i++ ) {
        rowCount.append( {i, 0} );
    }

    //int vtgSpeed = 5;
    if (current_speed != 0 && rowwidth != 0) {
        for (int i = 0; i < numRows1 && i < (sizeof(blockageSecCount1) / sizeof(blockageSecCount1[0])); i++)
            rowCount[k++].count = floor(blockageSecCount1[i] * 7.2 / rowwidth / current_speed);
        for (int i = 0; i < numRows2 && i < (sizeof(blockageSecCount2) / sizeof(blockageSecCount2[0])); i++)
            rowCount[k++].count = floor(blockageSecCount2[i] * 7.2 / rowwidth / current_speed);
        for (int i = 0; i < numRows3 && i < (sizeof(blockageSecCount3) / sizeof(blockageSecCount3[0])); i++)
            rowCount[k++].count = floor(blockageSecCount3[i] * 7.2 / rowwidth / current_speed);
        for (int i = 0; i < numRows4 && i < (sizeof(blockageSecCount4) / sizeof(blockageSecCount4[0])); i++)
            rowCount[k++].count = floor(blockageSecCount4[i] * 7.2 / rowwidth / current_speed);
    //}


        m_avg=0;
        for(int i = 0; i < numRows; i++) m_avg = m_avg + (rowCount[i].count); // среднее значение семян на гектар
        m_avg = m_avg / numRows; //bindable properties don't support /=
        m_max = 0;
        m_max_i=0;
        for (int i = 0; i < numRows; ++i) {
            if (rowCount[i].count > m_max) {
                m_max = (rowCount[i].count); // максимальное количество семян на гектар на сошнике
                m_max_i = i; // номер сошника на ктором максимальное значение
            }
        }
        m_min1 = 65535;
        m_min2 = 65535;
        m_min1_i = 0;
        m_min2_i = 0;

        for (int i = 0; i < numRows; ++i) {
            if (rowCount[i].count < m_min1) {
                m_min1 = (rowCount[i].count);
                m_min1_i = i;
            }
        }
        for(int i = 0; i < numRows; i++)
            if(rowCount[i].count < m_min2 && m_min1_i != i) // минимальные значения на гектар и номер сошника
            {
                m_min2 =(rowCount[i].count);
                m_min2_i = i;
            }
        m_blocked=0;
        for (int i = 0; i < numRows; i++)
            if (rowCount[i].count < blockCountMin) m_blocked = m_blocked + 1; // количество забитых сошников
    }
    m_blockageModel->setRows(rowCount); // обновляем модель

}
