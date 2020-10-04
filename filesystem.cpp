

class FileSystem{
public:
    virtual int open() = 0;
    virtual int read() = 0;
    virtual int write() = 0;
    virtual int close() = 0;
}