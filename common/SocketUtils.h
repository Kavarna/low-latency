#pragma once

#include "Logger.h"
#include <string>

const int MAX_TCP_SERVER_BACKLOG = 1024;

using Socket = int;

std::string GetIFaceIP(std::string const &iface);
bool SetNonBlocking(Socket fd);
bool SetNoDelay(Socket fd);
bool SetSOTimestamp(Socket fd);
bool SetMcastTTL(Socket fd, int ttl);
bool SetTTL(Socket fd, int ttl);
bool Join(Socket fd, std::string const &ip, std::string const &iface, int port);
Socket CreateSocket(QuickLogger &, std::string const &t_ip, std::string const &iface, int port, bool isUdp,
                    bool isListening, bool needsSOTimestamp);
