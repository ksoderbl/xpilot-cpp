#ifndef XPILOTFRAME_H
#define XPILOTFRAME_H

#include <vector>

class Position {
public:
    Position();
    Position(double x, double y);
    double x, y;
};


class Ball {
public:
    Ball(double x, double y, int id, int style);
    Position pos;
    int id;
    int style;
};

class Connector {
public:
    Connector(double x0, double y0, double x1, double y1, bool tractor);
    Position pos0, pos1;
    bool tractor;
};

class Ecm {
public:
    Ecm(double x, double y, int size);
    Position pos;
    int size;
};

class ItemType {
public:
    ItemType(double x, double y, int type);
    Position pos;
    int type;
};

class Laser {
public:
    Laser(double x, double y, int len, int dir, int color);
    Position pos;
    int len;
    int dir;
    int color;
};

class Mine {
public:
    Mine(double x, double y, int teammine, int id);
    Position pos;
    int teammine;
    int id;
};

class Missile {
public:
    Missile(double x, double y, int len, int dir);
    Position pos;
    int len;
    int dir;
};

class Refuel {
public:
    Refuel(double x0, double y0, double x1, double y1);
    Position pos0, pos1;
};

class Ship {
public:
    Ship(double x, double y, int id, double angle, bool shield, bool cloak, bool eshield, bool phased, bool deflector);
    Position pos;
    int id;
    double angle;
    bool shield, cloak, eshield;
    bool phased, deflector;
};

class Transporter {
public:
    Transporter(double x0, double y0, double x1, double y1);
    Position pos0, pos1;
};

class Wormhole {
public:
    Wormhole(double x, double y);
    Position pos;
};


class Frame {
public:
    std::vector<Ball> balls;
    std::vector<Connector> connectors;
    std::vector<Ecm> ecms;
    std::vector<ItemType> itemTypes;
    std::vector<Laser> lasers;
    std::vector<Mine> mines;
    std::vector<Missile> missiles;
    std::vector<Refuel> refuels;
    std::vector<Ship> ships;
    std::vector<Transporter> transporters;
    std::vector<Wormhole> wormholes;

    void clear();
};

extern Frame frame;



#endif
