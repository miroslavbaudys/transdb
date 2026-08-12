//
//  GeewaPacket.h
//  comLib
//
//  Created by Miroslav Kudrnac on 6/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef comLib_GeewaPacket_h
#define comLib_GeewaPacket_h

#include "../Defines.h"
#include "ByteBuffer.h"
#include "http_parser.h"

typedef std::map<std::string, std::string> HeaderMap;

class GeewaPacket
{
public:
    explicit GeewaPacket();
    ~GeewaPacket();
    
    //Add HTTP data to packet (auto parse)
    void AddHTTPData(char *pData, size_t dataSize);
    
    //clear packet for reuse
    void ClearPacket();
    
    //public declarations
    ByteBuffer  m_data;
    size_t      m_allDataCounter;
    uint16      m_opcode;
    bool        m_hasAllData;
    
private:
    //http parser
    http_parser             m_rHttp_parser;
    http_parser_settings    m_rHttp_parser_settings;
};

#endif
