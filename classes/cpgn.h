#ifndef CPGN_H
#define CPGN_H

#include <QObject>
#include <QByteArray>

//AutoSteerData
class CPGN_FE
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    enum CPGN_FE_Fields {
        speedLo = 5,
        speedHi = 6,
        status = 7,
        steerAngleLo = 8,
        steerAngleHi = 9,
        lineDistance  = 10,
        sc1to8 = 11,
        sc9to16 = 12
    };
    Q_ENUM(CPGN_FE_Fields)

    QByteArray pgn;
    CPGN_FE() : pgn(QByteArray("\x80\x81\x7f\xFE\x08\x00\x00\x00\x00\x00\x00\x00\x00\xCC",14 ))
    { }
    inline bool operator==(const CPGN_FE &other) const {
        return (this->pgn == other.pgn);
    }

    inline bool operator!=(const CPGN_FE &other) const {
        return !(this->pgn == other.pgn);
    }

};
Q_DECLARE_METATYPE(CPGN_FE)

class CPGN_FD
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    enum CPGN_FD_Fields {
        actualLo = 5,
        actualHi = 6,
        headLo = 7,
        headHi = 8,
        rollLo = 9,
        rollHi = 10,
        switchStatus = 11,
        pwm = 12
    };
    Q_ENUM(CPGN_FD_Fields)

    QByteArray pgn;

    CPGN_FD() : pgn(QByteArray( "\x80\x81\x7f\xfd\x08\x00\x00\x00\x00\x00\x00\x00\x00\xCC", 14))
    {}

    inline bool operator==(const CPGN_FD &other) const {
        return (this->pgn == other.pgn);
    }
    inline bool operator!=(const CPGN_FD &other) const {
        return !(this->pgn == other.pgn);
    }

};
Q_DECLARE_METATYPE(CPGN_FD)

//AutoSteer Settings
class CPGN_FC
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    enum CPGN_FC_Fields {
        gainProportional = 5,
        highPWM = 6,
        lowPWM = 7,
        minPWM = 8,
        countsPerDegree = 9,
        wasOffsetLo = 10,
        wasOffsetHi = 11,
        ackerman = 12
    };
    Q_ENUM(CPGN_FC_Fields)


    QByteArray pgn;

    CPGN_FC();
    inline bool operator==(const CPGN_FC &other) const {
        return (this->pgn == other.pgn);
    }
    inline bool operator!=(const CPGN_FC &other) const {
        return !(this->pgn == other.pgn);
    }
    void loadSettings();
};
Q_DECLARE_METATYPE(CPGN_FC)

//Autosteer Board Config
class CPGN_FB
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    enum CPGN_FB_Fields {
        set0 = 5,
        maxPulse = 6,
        minSpeed = 7,
        set1 = 8,
        angVel  = 9
    };

    Q_ENUM(CPGN_FB_Fields)

    QByteArray pgn;

    CPGN_FB() : pgn(QByteArray( "\x80\x81\x7f\xfb\x08\x00\x00\x00\x00\x00\x00\x00\x00\xCC", 14)) { }
    inline bool operator==(const CPGN_FB &other) const {
        return (this->pgn == other.pgn);
    }
    inline bool operator!=(const CPGN_FB &other) const {
        return !(this->pgn == other.pgn);
    }
};
Q_DECLARE_METATYPE(CPGN_FB)


// Machine Data
class CPGN_EF
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    QByteArray pgn;

    enum CPGN_EF_Fields {
        uturn = 5,
        speed = 6,
        hydLift = 7,
        tram = 8,
        geoStop = 9, //out of bounds etc
        sc1to8 = 11,
        sc9to16 = 12
    };

    Q_ENUM(CPGN_EF_Fields)

    CPGN_EF() : pgn(QByteArray( "\x80\x81\x7f\xef\x08\x00\x00\x00\x00\x00\x00\x00\x00\xCC", 14)) { }
    inline bool operator==(const CPGN_EF &other) const {
        return (this->pgn == other.pgn);
    }
    inline bool operator!=(const CPGN_EF &other) const {
        return !(this->pgn == other.pgn);
    }
};
Q_DECLARE_METATYPE(CPGN_EF)

class CPGN_E5
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    QByteArray pgn;

    enum CPGN_E5_Fields {
        sc1to8 = 5,
        sc9to16 = 6,
        sc17to24 = 7,
        sc25to32 = 8,
        sc33to40 = 9,
        sc41to48 = 10,
        sc49to56 = 11,
        sc57to64 = 12,
        toolLSpeed = 13,
        toolRSpeed = 14
    };
    Q_ENUM(CPGN_E5_Fields)

    CPGN_E5() : pgn(QByteArray( "\x80\x81\x7f\xe5\x0a\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xCC", 16)) { }
    inline bool operator==(const CPGN_E5 &other) const {
        return (this->pgn == other.pgn);
    }
    inline bool operator!=(const CPGN_E5 &other) const {
        return !(this->pgn == other.pgn);
    }
};
Q_DECLARE_METATYPE(CPGN_E5)

//Machine Config
class CPGN_EE
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    enum CPGN_EE_Fields {
        raiseTime = 5,
        lowerTime = 6,
        enableHyd = 7,
        set0 = 8,
        user1 = 9,
        user2 = 10,
        user3 = 11,
        user4  = 12,
    };
    Q_ENUM(CPGN_EE_Fields)

    QByteArray pgn;
    // PGN  - 127.239 0x7FEF
    //int crc = 0;

    CPGN_EE();
    inline bool operator==(const CPGN_EE &other) const {
        return (this->pgn == other.pgn);
    }
    inline bool operator!=(const CPGN_EE &other) const {
        return !(this->pgn == other.pgn);
    }
    void loadSettings();
    void MakeCRC();
};

Q_DECLARE_METATYPE(CPGN_EE)

//Relay Config
class CPGN_EC
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    QByteArray pgn;
    //where in the pgn is which pin

    enum CPGN_EC_Fields {
        pin0 = 5,
        pin1 = 6,
        pin2 = 7,
        pin3 = 8,
        pin4 = 9,
        pin5 = 10,
        pin6 = 11,
        pin7 = 12,
        pin8 = 13,
        pin9 = 14,

        pin10 = 15,
        pin11 = 16,
        pin12 = 17,
        pin13 = 18,
        pin14 = 19,
        pin15 = 20,
        pin16 = 21,

        pin17 = 22,
        pin18 = 23,
        pin19 = 24,
        pin20 = 25,
        pin21 = 26,
        pin22 = 27,
        pin23 = 28
    };

    Q_ENUM(CPGN_EC_Fields)

    // PGN  - 127.237 0x7FED
    CPGN_EC();
    inline bool operator==(const CPGN_EC &other) const {
        return (this->pgn == other.pgn);
    }
    inline bool operator!=(const CPGN_EC &other) const {
        return !(this->pgn == other.pgn);
    }
    void loadSettings();
    void MakeCRC();
    void Reset() {}
};
Q_DECLARE_METATYPE(CPGN_EC)

class CPGN_EB
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)

public:
    QByteArray pgn;

    //where in the pgn is which pin
    enum CPGN_EB_Fields {
        sec0Lo  = 5,
        sec1Lo  = 7,
        sec2Lo  = 9,
        sec3Lo  = 11,
        sec4Lo  = 13,
        sec5Lo  = 15,
        sec6Lo  = 17,
        sec7Lo  = 19,
        sec8Lo  = 21,
        sec9Lo  = 23,
        sec10Lo = 25,
        sec11Lo = 27,
        sec12Lo = 29,
        sec13Lo = 31,
        sec14Lo = 33,
        sec15Lo = 35,

        sec0Hi  = 6,
        sec1Hi  = 8,
        sec2Hi  = 10,
        sec3Hi  = 12,
        sec4Hi  = 14,
        sec5Hi  = 16,
        sec6Hi  = 18,
        sec7Hi  = 20,
        sec8Hi  = 22,
        sec9Hi  = 24,
        sec10Hi = 26,
        sec11Hi = 28,
        sec12Hi = 30,
        sec13Hi = 32,
        sec14Hi = 34,
        sec15Hi = 36,

        numSections = 37
    };
    Q_ENUM(CPGN_EB_Fields)

    CPGN_EB() : pgn(QByteArray( "\x80\x81\x7f\xeb\x21\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xCC", 39)) { }
    inline bool operator==(const CPGN_EB &other) const {
        return (this->pgn == other.pgn);
    }
    inline bool operator!=(const CPGN_EB &other) const {
        return !(this->pgn == other.pgn);
    }
    void Reset() {}
};
Q_DECLARE_METATYPE(CPGN_EB)

//Machine Config
class CPGN_F5
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)
public:
    QByteArray pgn;

    enum CPGN_F5_Fields {
        module_id = 5,
        module_rows = 6,
        min_countLO = 7,
        min_countHI = 8,
        max_countLO = 9,
        max_countHI = 10
    };
    Q_ENUM(CPGN_F5_Fields)

    CPGN_F5();
    inline bool operator==(const CPGN_F5 &other) const {
        return (this->pgn == other.pgn);
    }
    void MakeCRC();
};
Q_DECLARE_METATYPE(CPGN_F5)

//RateControl Config
class CPGN_F2
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)
public:
    QByteArray pgn;

    enum CPGN_F2_Fields {
        ID = 5,
        KP = 6,
        KI = 7,
        KD = 8,
        MinPWM = 9,
        MaxPWM = 10,
        PIDScale = 11
    };
    Q_ENUM(CPGN_F2_Fields)

    CPGN_F2();
    inline bool operator==(const CPGN_F2 &other) const {
        return (this->pgn == other.pgn);
    }
    void MakeCRC();
};
Q_DECLARE_METATYPE(CPGN_F2)

//Data to RateControl
class CPGN_F1
{
    Q_GADGET
    Q_PROPERTY(QByteArray pgn MEMBER pgn)
public:
    QByteArray pgn;

    enum CPGN_F1_Fields {
        ID = 5,
        RateSetLo = 6,
        RateSetHI = 7,
        FlowCalLO = 8,
        FlowCalHI = 9,
        Command = 10,
        ManualPWM = 11
    };
    Q_ENUM(CPGN_F1_Fields)

    CPGN_F1();
    inline bool operator==(const CPGN_F1 &other) const {
        return (this->pgn == other.pgn);
    }
    void MakeCRC();
};
Q_DECLARE_METATYPE(CPGN_F1)

#endif // CPGN_H
