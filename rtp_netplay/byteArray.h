//
// Created by xtt on 2020/1/4.
//

#ifndef TCPSERVER_BYTEARRAY_H
#define TCPSERVER_BYTEARRAY_H


class ByteArray {
public:
    ByteArray();
    ByteArray(const char* p,int size=-1);
    ByteArray(char c,int size);
    ByteArray(const ByteArray&);
    ~ByteArray();

    ByteArray& operator=(const ByteArray&);
    ByteArray& operator=(const char*);
    char operator[](int index);
private:
    char* pData=nullptr;
    int capacity=128;
    int dataSize=0;
private:
    void expand(int _size);//扩容
    void shrink();//压缩
public:
    ByteArray& append(char c);
    ByteArray& append(const char* s,int len=-1);
    ByteArray& append(const ByteArray& a);

    ByteArray& remove(int index,int len);
    ByteArray mid(int index,int len=-1)const;
    int size() const;
    bool isEmpty() const;
    void clear();

    char at(int index) const;
    char* data() const;
};


#endif //TCPSERVER_BYTEARRAY_H
