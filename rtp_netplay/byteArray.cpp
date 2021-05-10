//
// Created by xtt on 2020/1/4.
//

#include "byteArray.h"
#include <memory>
#include <cstring>
ByteArray::ByteArray(){
    pData=new char[capacity];
    memset(pData,0,capacity);
}

ByteArray::ByteArray(const char* p,int _size){
    if(!p)
        pData=nullptr;
    else if(_size==-1){
        int len=std::strlen(p);
        capacity=len+1;
        dataSize=len;
        pData=new char[capacity];
        memcpy(pData,p,dataSize);
        pData[len]='\0';
    }
    else if(_size>0){
        capacity=_size+1;
        pData=new char[capacity];
        memcpy(pData,p,_size);
        pData[_size]='\0';
        dataSize=_size;
    }
}

ByteArray::ByteArray(char c,int _size){
    if(dataSize<=0){
        pData=nullptr;
    }
    else{
        capacity=_size+1;
        pData=new char[capacity];
        memset(pData,c,_size);
        pData[_size]='\0';
        dataSize=capacity-1;
    }
}

ByteArray::ByteArray(const ByteArray& data){
    capacity=data.size()+1;
    pData=new char[capacity];
    memcpy(pData,data.data(),data.size());
    dataSize=data.size();
    pData[dataSize]='\0';
}

ByteArray::~ByteArray(){
    if(pData)
        delete [] pData;
}

ByteArray& ByteArray::operator=(const ByteArray& a){
    if(pData)
        delete [] pData;
    capacity = a.size() + 1;
    pData=new char[capacity];
    memcpy(pData,a.data(),a.size());
    dataSize=a.size();
    pData[dataSize]='\0';
    return *this;
}

ByteArray& ByteArray::operator=(const char* p){
    if(pData){
        delete [] pData;
    }
    if(!p){
        pData= nullptr;
        dataSize=0;
    }
    else{
        int len =std::strlen(p);
        capacity=len+1;
        dataSize=len;
        pData=new char[capacity];
        memcpy(pData,p,len+1);
    }
    return *this;
}

void ByteArray::expand(int _addSize){
    if(_addSize<capacity*2)
        capacity*=2;
    else
        capacity+=_addSize+1;
    char* pTemp=pData;
    pData=new char [capacity];
    memset(pData,0,capacity);
    memcpy(pData,pTemp,dataSize);
    delete [] pTemp;
}

void ByteArray::shrink(){
    capacity/=2;
    char* pTemp=pData;
    pData=new char[capacity];
    memset(pData,0,capacity);
    memcpy(pData,pTemp,dataSize);
    delete [] pTemp;
}

ByteArray& ByteArray::append(char c){
    if(dataSize+1>=capacity)
        expand(1);
    pData[dataSize]=c;
    dataSize++;
    pData[dataSize]='\0';

    return *this;
}

ByteArray& ByteArray::append(const char* s,int len){
    if(!s)
        return *this;
    if(len==-1){
        len=std::strlen(s);
    }
    if(dataSize+len>=capacity)
        expand(len);
    memcpy(pData+dataSize,s,len);
    dataSize+=len;
    pData[dataSize]='\0';

    return *this;
}

ByteArray& ByteArray::append(const ByteArray& a){
    if(a.isEmpty())
        return *this;
    if(dataSize+a.size()>=capacity)
        expand(a.size());
    memcpy(pData+dataSize,a.data(),a.size());
    dataSize+=a.size();
    pData[dataSize]='\0';

    return *this;
}

ByteArray& ByteArray::remove(int index,int len){
    if(len<0||index<0||(dataSize-index)<len){
        return *this;
    }
    memcpy(pData+index,pData+index+len,dataSize-index-len);
    dataSize-=len;
    if(capacity/4>dataSize)
        shrink();
    return *this;
}

ByteArray ByteArray::mid(int index,int len)const{
    if(index<0||(dataSize-index)<len){
        return *this;
    }
    ByteArray(pData+index,len);
    return ByteArray(pData+index,len);
}

int ByteArray::size() const {
    return dataSize;
}

bool ByteArray::isEmpty() const{
    return dataSize==0? true: false;
}

char ByteArray::at(int index) const{
    return pData[index];
}

char ByteArray::operator[](int index){
    return pData[index];
}

char* ByteArray::data() const{
    return pData;
}

void ByteArray::clear(){
    memset(pData,0,dataSize);
    dataSize=0;
}

