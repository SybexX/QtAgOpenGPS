#include "cpgn.h"
#include "classes/settingsmanager.h"

CPGN_FC::CPGN_FC(): pgn(QByteArray( "\x80\x81\x7f\xfc\x08\x00\x00\x00\x00\x00\x00\x00\x00\xCC", 14))
{
    loadSettings();
}

void CPGN_FC::loadSettings()
{
    pgn[gainProportional] = SettingsManager::instance()->as_Kp();
    pgn[highPWM] = SettingsManager::instance()->as_highSteerPWM();
    pgn[lowPWM] = SettingsManager::instance()->as_lowSteerPWM();
    pgn[minPWM] = SettingsManager::instance()->as_minSteerPWM();
    pgn[countsPerDegree] = SettingsManager::instance()->as_countsPerDegree();
    int wasOffset = (int)SettingsManager::instance()->as_wasOffset();
    pgn[wasOffsetHi] = (char)(wasOffset >> 8);
    pgn[wasOffsetLo] = (char)wasOffset;
    pgn[ackerman] = SettingsManager::instance()->as_ackerman();
}

CPGN_EE::CPGN_EE() : pgn(QByteArray("\x80\x81\x7f\xee\x08\x00\x00\x00\x00\x00\x00\x00\x00\xCC", 14))
{
    loadSettings();
}

void CPGN_EE::loadSettings()
{
    pgn[raiseTime] = SettingsManager::instance()->ardMac_hydRaiseTime();
    pgn[lowerTime] = SettingsManager::instance()->ardMac_hydLowerTime();
    pgn[enableHyd] = SettingsManager::instance()->ardMac_isHydEnabled();
    pgn[set0] = SettingsManager::instance()->ardMac_setting0();

    pgn[user1] = SettingsManager::instance()->ardMac_user1();
    pgn[user2] = SettingsManager::instance()->ardMac_user2();
    pgn[user3] = SettingsManager::instance()->ardMac_user3();
    pgn[user4] = SettingsManager::instance()->ardMac_user4();
}

void CPGN_EE::MakeCRC()
{
    int crc = 0;
    for (int i = 2; i < pgn.length() - 1; i++)
    {
        crc += pgn[i];
    }
    pgn[pgn.length() - 1] = crc;
}

CPGN_EC::CPGN_EC() : pgn(QByteArray("\x80\x81\x7f\xec\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xCC", 30))
{
    loadSettings();
}

void CPGN_EC::loadSettings()
{
    QVector<int> words = SettingsManager::instance()->relay_pinConfig();

    if (words.size() >= 24) {
        pgn[pin0] = words[0];
        pgn[pin1] = words[1];
        pgn[pin2] = words[2];
        pgn[pin3] = words[3];
        pgn[pin4] = words[4];
        pgn[pin5] = words[5];
        pgn[pin6] = words[6];
        pgn[pin7] = words[7];
        pgn[pin8] = words[8];
        pgn[pin9] = words[9];

        pgn[pin10] = words[10];
        pgn[pin11] = words[11];
        pgn[pin12] = words[12];
        pgn[pin13] = words[13];
        pgn[pin14] = words[14];
        pgn[pin15] = words[15];
        pgn[pin16] = words[16];
        pgn[pin17] = words[17];
        pgn[pin18] = words[18];
        pgn[pin19] = words[19];

        pgn[pin20] = words[20];
        pgn[pin21] = words[21];
        pgn[pin22] = words[22];
        pgn[pin23] = words[23];
    } else {
        qDebug() << "ERROR: relay_pinConfig is too small! Size:" << words.size() << "Expected: 24";
        // Valeurs par défaut
        for (int i = 0; i < 24; i++) {
            pgn[i] = 0;
        }
    }
}

void CPGN_EC::MakeCRC()
{
    int crc = 0;
    for (int i = 2; i < pgn.length() - 1; i++)
    {
        crc += pgn[i];
    }
    pgn[pgn.length() - 1] = crc;
}

CPGN_F1::CPGN_F1() : pgn(QByteArray("\x80\x81\x7f\xf1\x07\x00\x00\x00\x00\x00\x00\x00\xCC", 13))
{
}

void CPGN_F1::MakeCRC()
{
    int crc = 0;
    for (int i = 2; i < pgn.length() - 1; i++)
    {
        crc += pgn[i];
    }
    pgn[pgn.length() - 1] = crc;
}

CPGN_F2::CPGN_F2() : pgn(QByteArray("\x80\x81\x7f\xf2\x08\x00\x00\x00\x00\x00\x00\x00\xCC", 13))
{
}

void CPGN_F2::MakeCRC()
{
    int crc = 0;
    for (int i = 2; i < pgn.length() - 1; i++)
    {
        crc += pgn[i];
    }
    pgn[pgn.length() - 1] = crc;
}

CPGN_F5::CPGN_F5() : pgn(QByteArray("\x80\x81\x7f\xf5\x06\x00\x00\x00\x00\x00\x00\xCC", 12))
{
}

void CPGN_F5::MakeCRC()
{
    int crc = 0;
    for (int i = 2; i < pgn.length() - 1; i++)
    {
        crc += pgn[i];
    }
    pgn[pgn.length() - 1] = crc;
}
