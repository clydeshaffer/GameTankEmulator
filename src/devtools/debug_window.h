
class DebugWindow {
protected:
    bool open = true;
public:
    bool IsOpen();
    virtual void Draw() = 0;
};