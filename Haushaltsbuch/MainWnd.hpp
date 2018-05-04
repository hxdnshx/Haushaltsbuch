#pragma once

#include "MinimalIniFile.hpp"

#define WINDOW_CLASS _T("Haushaltsbuch")
#define WINDOW_TEXT  _T("滑稽簿 Rev.5.2")

bool MainWindow_Initialize();
bool MainWindow_PreTranslateMessage(MSG &msg);
