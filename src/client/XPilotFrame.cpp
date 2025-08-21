#include "XPilotFrame.h"

Frame frame;

Position::Position()
{
    this->x = 0;
    this->y = 0;
}

Position::Position(double x, double y)
{
    this->x = x;
    this->y = y;
}

Ball::Ball(double x, double y, int id, int style)
{
    Position pos(x, y);
    this->pos = pos;
    this->id = id;
    this->style = style;
}

Connector::Connector(double x0, double y0, double x1, double y1, bool tractor)
{
    Position pos0(x0, y0);
    Position pos1(x1, y1);
    this->pos0 = pos0;
    this->pos1 = pos1;
    this->tractor = tractor;
}

Ecm::Ecm(double x, double y, int size)
{
    Position pos(x, y);
    this->pos = pos;
    this->size = size;
}

ItemType::ItemType(double x, double y, int type)
{
    Position pos(x, y);
    this->pos = pos;
    this->type = type;
}

Laser::Laser(double x, double y, int len, int dir, int color)
{
    Position pos(x, y);
    this->pos = pos;
    this->len = len;
    this->dir = dir;
    this->color = color;
}

Mine::Mine(double x, double y, int teammine, int id)
{
    Position pos(x, y);
    this->pos = pos;
    this->teammine = teammine;
    this->id = id;
}

Missile::Missile(double x, double y, int len, int dir)
{
    Position pos(x, y);
    this->pos = pos;
    this->len = len;
    this->dir = dir;
}

Refuel::Refuel(double x0, double y0, double x1, double y1)
{
    Position pos0(x0, y0);
    Position pos1(x1, y1);
    this->pos0 = pos0;
    this->pos1 = pos1;
}

Ship::Ship(double x, double y, int id, double angle, bool shield, bool cloak, bool eshield, bool phased, bool deflector)
{
      Position pos(x, y);
    this->pos = pos;
    this->id = id;
    this->angle = angle;
    this->shield = shield;
    this->cloak = cloak;
    this->eshield = eshield;
    this->phased = phased;
    this->deflector = deflector;
}

Transporter::Transporter(double x0, double y0, double x1, double y1)
{
    Position pos0(x0, y0);
    Position pos1(x1, y1);
    this->pos0 = pos0;
    this->pos1 = pos1;
}

Wormhole::Wormhole(double x, double y)
{
    Position pos(x, y);
    this->pos = pos;
}

void Frame::clear()
{
    balls.clear();
    connectors.clear();
    ecms.clear();
    itemTypes.clear();
    lasers.clear();
    mines.clear();
    missiles.clear();
    refuels.clear();
    ships.clear();
    transporters.clear();
    wormholes.clear();
}
