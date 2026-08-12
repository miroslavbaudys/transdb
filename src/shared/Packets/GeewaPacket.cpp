//
//  GeewaPacket.cpp
//  comLib
//
//  Created by Miroslav Kudrnac on 6/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "GeewaPacket.h"

GeewaPacket::GeewaPacket() : m_allDataCounter(0), m_opcode(0), m_hasAllData(false)
{
    ClearPacket();
}

GeewaPacket::~GeewaPacket()
{
    
}

//Add data to HTTP packet (auto parse)
void GeewaPacket::AddHTTPData(char *pData, size_t dataSize)
{
    //data counter
    m_allDataCounter += dataSize;
    //exec parser
    http_parser_execute(&m_rHttp_parser, &m_rHttp_parser_settings, pData, dataSize);
}

static int GeewaPacket_on_body(http_parser *pParser, const char *at, size_t length)
{
    GeewaPacket *pGeewaPacket = static_cast<GeewaPacket*>(pParser->data);
    //append dat to buffer
    pGeewaPacket->m_data.append(at, length);
    return 0;
}

static int GeewaPacket_on_message_complete(http_parser *pParser)
{
    GeewaPacket *pGeewaPacket = static_cast<GeewaPacket*>(pParser->data);
    //set that we have all data
    pGeewaPacket->m_hasAllData = true;
    return 0;
}

void GeewaPacket::ClearPacket()
{
    //init http parser
    http_parser_init(&m_rHttp_parser, HTTP_RESPONSE);
    m_rHttp_parser.data = this;
    
    //set callbacks
    memset(&m_rHttp_parser_settings, 0, sizeof(http_parser_settings));
    m_rHttp_parser_settings.on_message_complete = &GeewaPacket_on_message_complete;
    m_rHttp_parser_settings.on_body = &GeewaPacket_on_body;
    
    //clear variables
    m_data.clear();
    m_hasAllData = false;
    m_allDataCounter = 0;
} 
