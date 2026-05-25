#pragma once

using namespace System;

typedef Object^ TiaPortalHandle;

TiaPortalHandle TiaBridge_Connect(String^ tiaVer);
void TiaBridge_Disconnect(TiaPortalHandle portal);

void TiaBridge_DoImport(TiaPortalHandle portal);
String^ TiaBridge_DoImportWithPath(TiaPortalHandle portal, String^ importPath);
void TiaBridge_DoExport(TiaPortalHandle portal);
void TiaBridge_DoRoundTripTest(TiaPortalHandle portal);

bool TiaBridge_IsAvailable();