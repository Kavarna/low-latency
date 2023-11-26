#!/bin/sh

meson setup build $1 -Dcpp_rtti=false # -Dcpp_eh=false
