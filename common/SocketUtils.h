#pragma once

#include "Logger.h"
#include <string>

const i32 MAX_TCP_SERVER_BACKLOG = 1024;

using Socket = int;

std::string GetIFaceIP(std::string const &iface);
bool SetNonBlocking(Socket fd);
bool SetNoDelay(Socket fd);
bool SetSOTimestamp(Socket fd);
bool SetMcastTTL(Socket fd, i32 ttl);
bool SetTTL(Socket fd, i32 ttl);
bool Join(Socket fd, std::string const &ip);
Socket CreateSocket(QuickLogger &, std::string const &t_ip, std::string const &iface, i32 port, bool isUdp,
                    bool isListening, bool needsSOTimestamp);
