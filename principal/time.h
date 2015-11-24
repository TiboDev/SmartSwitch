#ifndef TIME_H
#define TIME_H

using namespace std;

class CTime
{
  public:
    CTime();

    int getHour() const;
    int getMinute() const;
    int getSecond() const;

    bool hourChanged();
    bool minuteChanged();
    bool secondChanged();

    void setHour(int);
    void setMinute(int);
    void setSecond(int);

    bool isTimeSet();

  private:
    int hour;
    int minute;
    int second;
    int lastHour;
    int lastMinute;
    int lastSecond;
    bool timeSet;
};

#endif
