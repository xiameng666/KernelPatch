C:\Users\24151\Documents\BaiduSyncdisk\kernel_img>adb logcat *:E
--------- beginning of system
03-12 02:01:05.298   611   642 E keystore2: keystore2::maintenance: Call to earlyBootEnded failed for security level STRONGBOX: In call_with_watchdog: getting keymint device.
03-12 02:01:05.298   611   642 E keystore2: keystore2::error: In call_with_watchdog: getting keymint device
03-12 02:01:05.298   611   642 E keystore2:
03-12 02:01:05.298   611   642 E keystore2: Caused by:
03-12 02:01:05.298   611   642 E keystore2:     0: In get_keymint_device.
03-12 02:01:05.298   611   642 E keystore2:     1: In connect_keymint: Trying to get Legacy wrapper.
03-12 02:01:05.298   611   642 E keystore2:     2: Error::Km(ErrorCode(-68))
03-12 02:01:05.298   474   474 E vold    : keystore2 Keystore earlyBootEnded returned service specific error: -68
03-12 02:01:08.751  1186  1186 E BootLogoUpdater: [boot_logo_updater]fail to open: /sys/class/BOOT/BOOT/boot/boot_mode
03-12 02:01:08.751  1186  1186 E BootLogoUpdater: [boot_logo_updater]update boot reason = 0, ret = 0
03-12 02:01:08.841   834   834 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:10.940  1836  1836 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:11.689  1836  1870 E LocalDisplayAdapter: Can't find display mode with id -1
03-12 02:01:12.117  1836  1836 E PackageManager: updateSharedLibrariesLPr failed:
03-12 02:01:12.117  1836  1836 E PackageManager: com.android.server.pm.PackageManagerException: Package com.xiaomi.micloud.sdk requires unavailable shared library com.miui.system; failing!
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.SharedLibrariesImpl.collectSharedLibraryInfos(SharedLibrariesImpl.java:926)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.SharedLibrariesImpl.collectSharedLibraryInfos(SharedLibrariesImpl.java:867)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.SharedLibrariesImpl.updateSharedLibrariesLPw(SharedLibrariesImpl.java:532)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.InstallPackageHelper.commitPackageSettings(InstallPackageHelper.java:423)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.InstallPackageHelper.commitReconciledScanResultLocked(InstallPackageHelper.java:381)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.InstallPackageHelper.addForInitLI(InstallPackageHelper.java:3666)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.InstallPackageHelper.installPackagesFromDir(InstallPackageHelper.java:3508)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.InitAppsHelper.scanDirTracedLI(InitAppsHelper.java:418)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.InitAppsHelper.scanSystemDirs(InitAppsHelper.java:380)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.InitAppsHelper.initSystemApps(InitAppsHelper.java:199)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.PackageManagerService.<init>(PackageManagerService.java:2068)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.pm.PackageManagerService.main(PackageManagerService.java:1531)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.SystemServer.startBootstrapServices(SystemServer.java:1313)
03-12 02:01:12.117  1836  1836 E PackageManager:        at java.lang.reflect.Method.invoke(Native Method)
03-12 02:01:12.117  1836  1836 E PackageManager:        at kZWGXmv.I.WwemdE.tpt.U.TmUFZc.HookBridge.invokeOriginalMethod(Native Method)
03-12 02:01:12.117  1836  1836 E PackageManager:        at J.callback(Unknown Source:194)
03-12 02:01:12.117  1836  1836 E PackageManager:        at AndroidHelper_.startBootstrapServices(Unknown Source:11)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.SystemServer.run(SystemServer.java:1007)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.server.SystemServer.main(SystemServer.java:709)
03-12 02:01:12.117  1836  1836 E PackageManager:        at java.lang.reflect.Method.invoke(Native Method)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run(RuntimeInit.java:580)
03-12 02:01:12.117  1836  1836 E PackageManager:        at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:1006)
03-12 02:01:12.178  1836  1836 E PreinstallApp: Error occurs while read preinstalled PAI packages java.io.FileNotFoundException: /data/system/preinstallPAI.list: open failed: ENOENT (No such file or directory)
03-12 02:01:12.179  1836  1836 E PreinstallApp: no system property ro.appsflyer.preinstall.path
03-12 02:01:12.964  1836  1836 E PackageManager: There should probably be exactly one setup wizard; found 0: matches=[]
03-12 02:01:13.400  1836  1836 E MiuiMultiWindowUtils: initFreeFormResolutionArgs failed, device is rubens
03-12 02:01:13.452  1836  1836 E IntegrityFileManager: Error creating staging and rules directory
03-12 02:01:13.600  1836  1882 E PinnerService: Could not pin file /system/framework/miui-framework.jar
03-12 02:01:13.600  1836  1882 E PinnerService: android.system.ErrnoException: open failed: ENOENT (No such file or directory)
03-12 02:01:13.600  1836  1882 E PinnerService:         at libcore.io.Linux.open(Native Method)
03-12 02:01:13.600  1836  1882 E PinnerService:         at libcore.io.ForwardingOs.open(ForwardingOs.java:563)
03-12 02:01:13.600  1836  1882 E PinnerService:         at libcore.io.BlockGuardOs.open(BlockGuardOs.java:274)
03-12 02:01:13.600  1836  1882 E PinnerService:         at android.system.Os.open(Os.java:494)
03-12 02:01:13.600  1836  1882 E PinnerService:         at com.android.server.PinnerService.pinFileRanges(PinnerService.java:975)
03-12 02:01:13.600  1836  1882 E PinnerService:         at com.android.server.PinnerService.pinFile(PinnerService.java:850)
03-12 02:01:13.600  1836  1882 E PinnerService:         at com.android.server.PinnerService.handlePinOnStart(PinnerService.java:305)
03-12 02:01:13.600  1836  1882 E PinnerService:         at com.android.server.PinnerService.-$$Nest$mhandlePinOnStart(Unknown Source:0)
03-12 02:01:13.600  1836  1882 E PinnerService:         at com.android.server.PinnerService$PinnerHandler.handleMessage(PinnerService.java:1219)
03-12 02:01:13.600  1836  1882 E PinnerService:         at android.os.Handler.dispatchMessage(Handler.java:106)
03-12 02:01:13.600  1836  1882 E PinnerService:         at android.os.Looper.loopOnce(Looper.java:210)
03-12 02:01:13.600  1836  1882 E PinnerService:         at android.os.Looper.loop(Looper.java:299)
03-12 02:01:13.600  1836  1882 E PinnerService:         at android.os.HandlerThread.run(HandlerThread.java:67)
03-12 02:01:13.602  1836  1882 E PinnerService: Failed to pin file = /system/framework/miui-framework.jar
03-12 02:01:13.953  1836  1884 E PerfShielderService: [Ljava.lang.StackTraceElement;@33cf994
03-12 02:01:13.999  1399  1864 E BootAnimation: displays id for the value of system prop:
03-12 02:01:14.002  1836  1836 E FileUtils: java.io.FileNotFoundException: /vendor/etc/game_memory_cleaner.cfg: open failed: ENOENT (No such file or directory)
03-12 02:01:14.002  1836  1836 E FileUtils:     at libcore.io.IoBridge.open(IoBridge.java:574)
03-12 02:01:14.002  1836  1836 E FileUtils:     at java.io.FileInputStream.<init>(FileInputStream.java:160)
03-12 02:01:14.002  1836  1836 E FileUtils:     at java.io.FileInputStream.<init>(FileInputStream.java:115)
03-12 02:01:14.002  1836  1836 E FileUtils:     at java.io.FileReader.<init>(FileReader.java:60)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.miui.server.migard.utils.FileUtils.readFromSys(FileUtils.java:78)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.miui.server.migard.utils.FileUtils.readFromSys(FileUtils.java:70)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.miui.server.migard.memory.GameMemoryCleanerConfig.configFromFile(GameMemoryCleanerConfig.java:176)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.miui.server.migard.memory.GameMemoryCleaner.<init>(GameMemoryCleaner.java:122)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.miui.server.migard.MiGardService.<init>(MiGardService.java:40)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.miui.server.migard.MiGardService.startService(MiGardService.java:58)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.android.server.SystemServerImpl.addExtraServices(SystemServerImpl.java:233)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.android.server.SystemServer.startOtherServices(SystemServer.java:2393)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.android.server.SystemServer.run(SystemServer.java:1013)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.android.server.SystemServer.main(SystemServer.java:709)
03-12 02:01:14.002  1836  1836 E FileUtils:     at java.lang.reflect.Method.invoke(Native Method)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run(RuntimeInit.java:580)
03-12 02:01:14.002  1836  1836 E FileUtils:     at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:1006)
03-12 02:01:14.002  1836  1836 E FileUtils: Caused by: android.system.ErrnoException: open failed: ENOENT (No such file or directory)
03-12 02:01:14.002  1836  1836 E FileUtils:     at libcore.io.Linux.open(Native Method)
03-12 02:01:14.002  1836  1836 E FileUtils:     at libcore.io.ForwardingOs.open(ForwardingOs.java:563)
03-12 02:01:14.002  1836  1836 E FileUtils:     at libcore.io.BlockGuardOs.open(BlockGuardOs.java:274)
03-12 02:01:14.002  1836  1836 E FileUtils:     at libcore.io.IoBridge.open(IoBridge.java:560)
03-12 02:01:14.002  1836  1836 E FileUtils:     ... 16 more
03-12 02:01:14.059  1836  1836 E AutofillManagerServiceImpl: Bad service name: com.miui.contentcatcher/.autofill.services.MiuiAutofillService
03-12 02:01:14.063  1836  1836 E MtkSystemServerImpl: com.mediatek.fmradio.FmRadioPackageManager not found
03-12 02:01:14.077  1836  1836 E StatsPullAtomCallbackImpl: Failed to start PowerStatsService statsd pullers
03-12 02:01:14.078  1836  1836 E BatteryStatsService: Could not register PowerStatsInternal
03-12 02:01:14.086  1836  1836 E DisplayPowerController[0]: failed to set up display white-balance: java.lang.IllegalStateException: cannot find sensor com.google.sensor.color
03-12 02:01:14.147  1836  1836 E MiuiPadKeyboardManager: notSupport any keyboard!
03-12 02:01:14.233  1836  1985 E DefaultPermGrantPolicy: Package not found: com.google.android.apps.restore
03-12 02:01:14.238  1836  1870 E DisplayModeDirector: Asked about unknown display, returning empty display mode specs!(id=0)
03-12 02:01:14.239  1836  1870 E DisplayModeDirector: Asked about unknown display, returning empty display mode specs!(id=0)
03-12 02:01:14.241  1836  1836 E SystemServer: BOOT FAILURE starting com.miui.me.server.auto_install.InstallService
03-12 02:01:14.241  1836  1836 E SystemServer: java.lang.RuntimeException: Failed to create service com.miui.me.server.auto_install.InstallService from class loader dalvik.system.PathClassLoader[DexPathList[[zip file "/system/framework/com.android.location.provider.jar", zip file "/system/framework/services.jar", zip file "/system_ext/framework/miui-services.jar", zip file "/apex/com.android.adservices/javalib/service-adservices.jar", zip file "/apex/com.android.adservices/javalib/service-sdksandbox.jar", zip file "/apex/com.android.appsearch/javalib/service-appsearch.jar", zip file "/apex/com.android.art/javalib/service-art.jar", zip file "/apex/com.android.media/javalib/service-media-s.jar", zip file "/apex/com.android.permission/javalib/service-permission.jar", zip file "/system/framework/miuix.jar"],nativeLibraryDirectories=[/system/lib64, /system_ext/lib64, /system/lib64, /system_ext/lib64]]]: service class not found, usually indicates that the caller should have called PackageManager.hasSystemFeature() to check whether the feature is available on this device before trying to start the services that implement it. Also ensure that the correct path for the classloader is supplied, if applicable.
03-12 02:01:14.241  1836  1836 E SystemServer:  at com.android.server.SystemServiceManager.loadClassFromLoader(SystemServiceManager.java:199)
03-12 02:01:14.241  1836  1836 E SystemServer:  at com.android.server.SystemServiceManager.startService(SystemServiceManager.java:144)
03-12 02:01:14.241  1836  1836 E SystemServer:  at com.android.server.SystemServer.startOtherServices(SystemServer.java:2877)
03-12 02:01:14.241  1836  1836 E SystemServer:  at com.android.server.SystemServer.run(SystemServer.java:1013)
03-12 02:01:14.241  1836  1836 E SystemServer:  at com.android.server.SystemServer.main(SystemServer.java:709)
03-12 02:01:14.241  1836  1836 E SystemServer:  at java.lang.reflect.Method.invoke(Native Method)
03-12 02:01:14.241  1836  1836 E SystemServer:  at com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run(RuntimeInit.java:580)
03-12 02:01:14.241  1836  1836 E SystemServer:  at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:1006)
03-12 02:01:14.241  1836  1836 E SystemServer: Caused by: java.lang.ClassNotFoundException: com.miui.me.server.auto_install.InstallService
03-12 02:01:14.241  1836  1836 E SystemServer:  at java.lang.Class.classForName(Native Method)
03-12 02:01:14.241  1836  1836 E SystemServer:  at java.lang.Class.forName(Class.java:454)
03-12 02:01:14.241  1836  1836 E SystemServer:  at com.android.server.SystemServiceManager.loadClassFromLoader(SystemServiceManager.java:196)
03-12 02:01:14.241  1836  1836 E SystemServer:  ... 7 more
03-12 02:01:14.241  1836  1836 E SystemServer: Caused by: java.lang.ClassNotFoundException: com.miui.me.server.auto_install.InstallService
03-12 02:01:14.241  1836  1836 E SystemServer:  ... 10 more
03-12 02:01:14.271  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_755905933/u0
03-12 02:01:14.271  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_755910933/u0
03-12 02:01:14.271  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_755913233/u0
03-12 02:01:14.271  1836  1836 E ActivityManager: Unable to find org.telegram.messenger/u0
03-12 02:01:14.272  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_744404833/u0
03-12 02:01:14.272  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_744410233/u0
03-12 02:01:14.272  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_744415833/u0
03-12 02:01:14.272  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_744415933/u0
03-12 02:01:14.272  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_744417133/u0
03-12 02:01:14.273  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_749903433/u0
03-12 02:01:14.273  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_749905233/u0
03-12 02:01:14.273  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_749910933/u0
03-12 02:01:14.273  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_749914633/u0
03-12 02:01:14.273  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_749919233/u0
03-12 02:01:14.274  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_739009733/u0
03-12 02:01:14.274  1836  1836 E ActivityManager: Unable to find com.google.android.trichromelibrary_739012233/u0
03-12 02:01:14.280  1836  1870 E LocalDisplayAdapter: Can't find display mode with id -1
03-12 02:01:14.284  1836  1962 E AppStandbyController: Tried to restrict app com.google.android.documentsui for an unsupported reason
03-12 02:01:14.343  1836  2060 E NativeTombstoneManager: Tombstone's UID (99019) not an app, ignoring
03-12 02:01:14.344  1836  2060 E NativeTombstoneManager: Tombstone's UID (99019) not an app, ignoring
03-12 02:01:14.426  1836  2060 E NativeTombstoneManager: Tombstone's UID (0) not an app, ignoring
03-12 02:01:14.427  1836  2060 E NativeTombstoneManager: Tombstone's UID (0) not an app, ignoring
03-12 02:01:14.431  1836  2060 E NativeTombstoneManager: Tombstone's UID (0) not an app, ignoring
03-12 02:01:14.432  1836  2060 E NativeTombstoneManager: Tombstone's UID (0) not an app, ignoring
03-12 02:01:14.433  1836  2060 E NativeTombstoneManager: Tombstone's UID (0) not an app, ignoring
03-12 02:01:14.433  1836  2060 E NativeTombstoneManager: Tombstone's UID (0) not an app, ignoring
03-12 02:01:14.580  2302  2302 E android.hardware.wifi@1.0-service-lazy: Unknown iface name: wlan0
03-12 02:01:14.580  2302  2302 E android.hardware.wifi@1.0-service-lazy: Unknown iface name: wlan0
03-12 02:01:14.582  2302  2302 E android.hardware.wifi@1.0-service-lazy: Unknown iface name: wlan0
03-12 02:01:14.582  2302  2302 E android.hardware.wifi@1.0-service-lazy: Unknown iface name: wlan0
03-12 02:01:14.584  2302  2302 E android.hardware.wifi@1.0-service-lazy: Unknown iface name: wlan0
03-12 02:01:14.584  2302  2302 E android.hardware.wifi@1.0-service-lazy: Unknown iface name: wlan0
03-12 02:01:14.590  2302  2302 E android.hardware.wifi@1.0-service-lazy: Failed to register radio mode change callback
03-12 02:01:14.744  2389  2389 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:14.851  1836  1836 E WallpaperManagerService: Unable to apply new wallpaper
03-12 02:01:14.891  2448  2448 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:14.990  1836  2061 E AppOps  : Bad call made by uid 1000. Package "android" does not belong to uid 1037.
03-12 02:01:14.990  1836  2061 E AppOps  : checkOperation
03-12 02:01:14.990  1836  2061 E AppOps  : java.lang.SecurityException: Specified package "android" under uid 1037 but it is not
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.verifyAndGetBypass(AppOpsService.java:4879)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.verifyAndGetBypass(AppOpsService.java:4747)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperationUnchecked(AppOpsService.java:3291)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperationImpl(AppOpsService.java:3274)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.-$$Nest$mcheckOperationImpl(Unknown Source:0)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher.lambda$checkOperation$0(AppOpsService.java:7787)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher$$ExternalSyntheticLambda13.apply(Unknown Source:25)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.policy.AppOpsPolicy.checkOperation(AppOpsPolicy.java:213)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher.checkOperation(AppOpsService.java:7786)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperation(AppOpsService.java:3261)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.StorageManagerService$StorageManagerInternalImpl.hasExternalStorageAccess(StorageManagerService.java:5003)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.am.ProcessList.startProcessLocked(ProcessList.java:1759)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.am.ProcessList.startProcessLocked(ProcessList.java:2434)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.am.ProcessList.startProcessLocked(ProcessList.java:2587)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.am.ActivityManagerService.startIsolatedProcess(ActivityManagerService.java:2906)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.am.ActivityManagerService$LocalService.startIsolatedProcess(ActivityManagerService.java:16960)
03-12 02:01:14.990  1836  2061 E AppOps  :      at android.webkit.WebViewLibraryLoader.createRelroFile(WebViewLibraryLoader.java:126)
03-12 02:01:14.990  1836  2061 E AppOps  :      at android.webkit.WebViewLibraryLoader.createRelros(WebViewLibraryLoader.java:164)
03-12 02:01:14.990  1836  2061 E AppOps  :      at android.webkit.WebViewLibraryLoader.prepareNativeLibraries(WebViewLibraryLoader.java:152)
03-12 02:01:14.990  1836  2061 E AppOps  :      at android.webkit.WebViewFactory.onWebViewProviderChanged(WebViewFactory.java:555)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.webkit.SystemImpl.onWebViewProviderChanged(SystemImpl.java:174)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.webkit.WebViewUpdateServiceImpl.onWebViewProviderChanged(WebViewUpdateServiceImpl.java:351)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.webkit.WebViewUpdateServiceImpl.prepareWebViewInSystemServer(WebViewUpdateServiceImpl.java:182)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.webkit.WebViewUpdateService.prepareWebViewInSystemServer(WebViewUpdateService.java:129)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.SystemServer.lambda$startOtherServices$3$com-android-server-SystemServer(SystemServer.java:2977)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.SystemServer$$ExternalSyntheticLambda6.run(Unknown Source:2)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.SystemServerInitThreadPool.lambda$submitTask$0$com-android-server-SystemServerInitThreadPool(SystemServerInitThreadPool.java:106)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.server.SystemServerInitThreadPool$$ExternalSyntheticLambda0.run(Unknown Source:6)
03-12 02:01:14.990  1836  2061 E AppOps  :      at java.util.concurrent.Executors$RunnableAdapter.call(Executors.java:463)
03-12 02:01:14.990  1836  2061 E AppOps  :      at java.util.concurrent.FutureTask.run(FutureTask.java:264)
03-12 02:01:14.990  1836  2061 E AppOps  :      at java.util.concurrent.ThreadPoolExecutor.runWorker(ThreadPoolExecutor.java:1137)
03-12 02:01:14.990  1836  2061 E AppOps  :      at java.util.concurrent.ThreadPoolExecutor$Worker.run(ThreadPoolExecutor.java:637)
03-12 02:01:14.990  1836  2061 E AppOps  :      at com.android.internal.util.ConcurrentUtils$1$1.run(ConcurrentUtils.java:65)
03-12 02:01:14.992  1836  2061 E AppOps  : Bad call made by uid 1000. Package "android" does not belong to uid 1037.
03-12 02:01:14.992  1836  2061 E AppOps  : checkOperation
03-12 02:01:14.992  1836  2061 E AppOps  : java.lang.SecurityException: Specified package "android" under uid 1037 but it is not
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.verifyAndGetBypass(AppOpsService.java:4879)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.verifyAndGetBypass(AppOpsService.java:4747)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperationUnchecked(AppOpsService.java:3291)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperationImpl(AppOpsService.java:3274)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.-$$Nest$mcheckOperationImpl(Unknown Source:0)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher.lambda$checkOperation$0(AppOpsService.java:7787)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher$$ExternalSyntheticLambda13.apply(Unknown Source:25)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.policy.AppOpsPolicy.checkOperation(AppOpsPolicy.java:213)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher.checkOperation(AppOpsService.java:7786)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperation(AppOpsService.java:3261)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.StorageManagerService$StorageManagerInternalImpl.hasExternalStorageAccess(StorageManagerService.java:5003)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.am.ProcessList.startProcessLocked(ProcessList.java:1759)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.am.ProcessList.startProcessLocked(ProcessList.java:2434)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.am.ProcessList.startProcessLocked(ProcessList.java:2587)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.am.ActivityManagerService.startIsolatedProcess(ActivityManagerService.java:2906)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.am.ActivityManagerService$LocalService.startIsolatedProcess(ActivityManagerService.java:16960)
03-12 02:01:14.992  1836  2061 E AppOps  :      at android.webkit.WebViewLibraryLoader.createRelroFile(WebViewLibraryLoader.java:126)
03-12 02:01:14.992  1836  2061 E AppOps  :      at android.webkit.WebViewLibraryLoader.createRelros(WebViewLibraryLoader.java:170)
03-12 02:01:14.992  1836  2061 E AppOps  :      at android.webkit.WebViewLibraryLoader.prepareNativeLibraries(WebViewLibraryLoader.java:152)
03-12 02:01:14.992  1836  2061 E AppOps  :      at android.webkit.WebViewFactory.onWebViewProviderChanged(WebViewFactory.java:555)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.webkit.SystemImpl.onWebViewProviderChanged(SystemImpl.java:174)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.webkit.WebViewUpdateServiceImpl.onWebViewProviderChanged(WebViewUpdateServiceImpl.java:351)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.webkit.WebViewUpdateServiceImpl.prepareWebViewInSystemServer(WebViewUpdateServiceImpl.java:182)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.webkit.WebViewUpdateService.prepareWebViewInSystemServer(WebViewUpdateService.java:129)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.SystemServer.lambda$startOtherServices$3$com-android-server-SystemServer(SystemServer.java:2977)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.SystemServer$$ExternalSyntheticLambda6.run(Unknown Source:2)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.SystemServerInitThreadPool.lambda$submitTask$0$com-android-server-SystemServerInitThreadPool(SystemServerInitThreadPool.java:106)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.server.SystemServerInitThreadPool$$ExternalSyntheticLambda0.run(Unknown Source:6)
03-12 02:01:14.992  1836  2061 E AppOps  :      at java.util.concurrent.Executors$RunnableAdapter.call(Executors.java:463)
03-12 02:01:14.992  1836  2061 E AppOps  :      at java.util.concurrent.FutureTask.run(FutureTask.java:264)
03-12 02:01:14.992  1836  2061 E AppOps  :      at java.util.concurrent.ThreadPoolExecutor.runWorker(ThreadPoolExecutor.java:1137)
03-12 02:01:14.992  1836  2061 E AppOps  :      at java.util.concurrent.ThreadPoolExecutor$Worker.run(ThreadPoolExecutor.java:637)
03-12 02:01:14.992  1836  2061 E AppOps  :      at com.android.internal.util.ConcurrentUtils$1$1.run(ConcurrentUtils.java:65)
03-12 02:01:15.297  1836  2511 E WindowManager: Window Session Crash
03-12 02:01:15.297  1836  2511 E WindowManager: java.lang.IllegalArgumentException: Requested window android.os.BinderProxy@b51794 does not exist
03-12 02:01:15.297  1836  2511 E WindowManager:         at com.android.server.wm.WindowManagerService.windowForClientLocked(WindowManagerService.java:6306)
03-12 02:01:15.297  1836  2511 E WindowManager:         at com.android.server.wm.Session.actionOnWallpaper(Session.java:583)
03-12 02:01:15.297  1836  2511 E WindowManager:         at com.android.server.wm.Session.setShouldZoomOutWallpaper(Session.java:620)
03-12 02:01:15.297  1836  2511 E WindowManager:         at android.view.IWindowSession$Stub.onTransact(IWindowSession.java:983)
03-12 02:01:15.297  1836  2511 E WindowManager:         at com.android.server.wm.Session.onTransact(Session.java:191)
03-12 02:01:15.297  1836  2511 E WindowManager:         at android.os.Binder.execTransactInternal(Binder.java:1290)
03-12 02:01:15.297  1836  2511 E WindowManager:         at android.os.Binder.execTransact(Binder.java:1249)
03-12 02:01:15.370  1836  2235 E AppOps  : Bad call made by uid 1000. Package "android" does not belong to uid 1037.
03-12 02:01:15.371  1836  2235 E AppOps  : checkOperation
03-12 02:01:15.371  1836  2235 E AppOps  : java.lang.SecurityException: Specified package "android" under uid 1037 but it is not
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.verifyAndGetBypass(AppOpsService.java:4879)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.verifyAndGetBypass(AppOpsService.java:4747)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperationUnchecked(AppOpsService.java:3291)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperationImpl(AppOpsService.java:3274)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.-$$Nest$mcheckOperationImpl(Unknown Source:0)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher.lambda$checkOperation$0(AppOpsService.java:7787)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher$$ExternalSyntheticLambda13.apply(Unknown Source:25)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.policy.AppOpsPolicy.checkOperation(AppOpsPolicy.java:213)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher.checkOperation(AppOpsService.java:7786)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperation(AppOpsService.java:3261)
03-12 02:01:15.371  1836  2235 E AppOps  :      at android.app.AppOpsManager.checkOpNoThrow(AppOpsManager.java:9093)
03-12 02:01:15.371  1836  2235 E AppOps  :      at android.miui.AppOpsUtils.getApplicationAutoStart(AppOpsUtils.java:84)
03-12 02:01:15.371  1836  2235 E AppOps  :      at com.android.server.am.AppStateManager$AppState$RunningProcess$1.run(AppStateManager.java:1663)
03-12 02:01:15.371  1836  2235 E AppOps  :      at android.os.Handler.handleCallback(Handler.java:942)
03-12 02:01:15.371  1836  2235 E AppOps  :      at android.os.Handler.dispatchMessage(Handler.java:99)
03-12 02:01:15.371  1836  2235 E AppOps  :      at android.os.Looper.loopOnce(Looper.java:210)
03-12 02:01:15.371  1836  2235 E AppOps  :      at android.os.Looper.loop(Looper.java:299)
03-12 02:01:15.371  1836  2235 E AppOps  :      at android.os.HandlerThread.run(HandlerThread.java:67)
03-12 02:01:15.377  2537  2537 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.386  1836  2235 E AppOps  : Bad call made by uid 1000. Package "android" does not belong to uid 1037.
03-12 02:01:15.386  1836  2235 E AppOps  : checkOperation
03-12 02:01:15.386  1836  2235 E AppOps  : java.lang.SecurityException: Specified package "android" under uid 1037 but it is not
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.verifyAndGetBypass(AppOpsService.java:4879)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.verifyAndGetBypass(AppOpsService.java:4747)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperationUnchecked(AppOpsService.java:3291)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperationImpl(AppOpsService.java:3274)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.-$$Nest$mcheckOperationImpl(Unknown Source:0)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher.lambda$checkOperation$0(AppOpsService.java:7787)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher$$ExternalSyntheticLambda13.apply(Unknown Source:25)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.policy.AppOpsPolicy.checkOperation(AppOpsPolicy.java:213)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService$CheckOpsDelegateDispatcher.checkOperation(AppOpsService.java:7786)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.appop.AppOpsService.checkOperation(AppOpsService.java:3261)
03-12 02:01:15.386  1836  2235 E AppOps  :      at android.app.AppOpsManager.checkOpNoThrow(AppOpsManager.java:9093)
03-12 02:01:15.386  1836  2235 E AppOps  :      at android.miui.AppOpsUtils.getApplicationAutoStart(AppOpsUtils.java:84)
03-12 02:01:15.386  1836  2235 E AppOps  :      at com.android.server.am.AppStateManager$AppState$RunningProcess$1.run(AppStateManager.java:1663)
03-12 02:01:15.386  1836  2235 E AppOps  :      at android.os.Handler.handleCallback(Handler.java:942)
03-12 02:01:15.386  1836  2235 E AppOps  :      at android.os.Handler.dispatchMessage(Handler.java:99)
03-12 02:01:15.386  1836  2235 E AppOps  :      at android.os.Looper.loopOnce(Looper.java:210)
03-12 02:01:15.386  1836  2235 E AppOps  :      at android.os.Looper.loop(Looper.java:299)
03-12 02:01:15.386  1836  2235 E AppOps  :      at android.os.HandlerThread.run(HandlerThread.java:67)
03-12 02:01:15.390  2536  2536 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.415  2549  2549 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.446  2575  2575 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.451  2579  2579 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.455  2587  2587 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.491  2603  2603 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.509  2626  2626 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.549  2642  2642 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.573  2681  2681 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.604  2642  2642 E LoadedApk: Unable to instantiate appComponentFactory
03-12 02:01:15.604  2642  2642 E LoadedApk: java.lang.ClassNotFoundException: Didn't find class "androidx.core.app.CoreComponentFactory" on path: DexPathList[[zip file "/data/app/~~9O7xoutxmiOGVQ_P8jRxJw==/com.miui.home-w0qhSm_L8kTAEWLJcJhBLw==/base.apk"],nativeLibraryDirectories=[/data/app/~~9O7xoutxmiOGVQ_P8jRxJw==/com.miui.home-w0qhSm_L8kTAEWLJcJhBLw==/lib/arm64, /data/app/~~9O7xoutxmiOGVQ_P8jRxJw==/com.miui.home-w0qhSm_L8kTAEWLJcJhBLw==/base.apk!/lib/arm64-v8a, /system_ext/app/miuisystem/miuisystem.apk!/lib/arm64-v8a, /system_ext/app/miui/miui.apk!/lib/arm64-v8a, /system/lib64, /system_ext/lib64]]
03-12 02:01:15.604  2642  2642 E LoadedApk:     at dalvik.system.BaseDexClassLoader.findClass(BaseDexClassLoader.java:259)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at java.lang.ClassLoader.loadClass(ClassLoader.java:379)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at java.lang.ClassLoader.loadClass(ClassLoader.java:312)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.LoadedApk.createAppFactory(LoadedApk.java:286)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.LoadedApk.createOrUpdateClassLoaderLocked(LoadedApk.java:1056)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.LoadedApk.getClassLoader(LoadedApk.java:1143)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.LoadedApk.getResources(LoadedApk.java:1402)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.ContextImpl.createAppContext(ContextImpl.java:3101)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.ContextImpl.createAppContext(ContextImpl.java:3093)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.ActivityThread.handleBindApplication(ActivityThread.java:6868)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.ActivityThread.-$$Nest$mhandleBindApplication(Unknown Source:0)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.ActivityThread$H.handleMessage(ActivityThread.java:2228)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.os.Handler.dispatchMessage(Handler.java:106)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.os.Looper.loopOnce(Looper.java:210)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.os.Looper.loop(Looper.java:299)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at android.app.ActivityThread.main(ActivityThread.java:8130)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at java.lang.reflect.Method.invoke(Native Method)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run(RuntimeInit.java:580)
03-12 02:01:15.604  2642  2642 E LoadedApk:     at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:1028)
03-12 02:01:15.703  2642  2642 E MiuiMultiWindowUtils: initFreeFormResolutionArgs failed, device is rubens
03-12 02:01:15.855  2818  2818 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:15.935  1836  1836 E AutofillManagerServiceImpl: Bad service name: com.miui.contentcatcher/.autofill.services.MiuiAutofillService
03-12 02:01:16.159  2962  2962 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:16.292  1836  1870 E WindowManager: camera_covered_service start again!
03-12 02:01:16.345  1836  1882 E CountryDetector: Could not instantiate the custom country detector class
03-12 02:01:16.418  3037  3037 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:16.510  3080  3080 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:16.529  1836  2054 E JobScheduler.JobStatus: App com.android.htmlviewer became active but still in NEVER bucket
03-12 02:01:16.530  1836  2054 E JobScheduler.JobStatus: App com.android.htmlviewer became active but still in NEVER bucket
03-12 02:01:16.673  2603  2603 E MiuiMultiWindowUtils: initFreeFormResolutionArgs failed, device is rubens
03-12 02:01:16.692  1836  1857 E PackageDexUsage: Unsupported context?
03-12 02:01:16.714  2626  2626 E MiuiMultiWindowUtils: initFreeFormResolutionArgs failed, device is rubens
03-12 02:01:17.276  1836  1882 E StatsPullAtomService: subInfo of subId 1 is invalid, ignored.
03-12 02:01:17.295  1836  1882 E StatsPullAtomService: subInfo of subId 1 is invalid, ignored.
03-12 02:01:17.362  3258  3258 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:17.373  1836  1882 E StatsPullAtomService: subInfo of subId 1 is invalid, ignored.
03-12 02:01:17.384  1836  2997 E SettingsProvider: Requested user 10 does not exist
03-12 02:01:18.202  3350  3350 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:19.071  3410  3410 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:19.136  1836  1870 E PowerStatsService: Failed to start PowerStatsService loggers
03-12 02:01:19.167  3437  3437 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:19.190  1836  1870 E SystemServiceRegistry: Manager wrapper not available: contexthub
03-12 02:01:19.444  3488  3488 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:19.479  3511  3511 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:19.493  3535  3535 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:19.624  1836  1857 E PackageDexUsage: Unsupported context?
03-12 02:01:19.641  1836  1857 E PackageDexUsage: Unsupported context?
03-12 02:01:19.797  3511  3511 E MiuiMultiWindowUtils: initFreeFormResolutionArgs failed, device is rubens
--------- beginning of main
03-12 02:01:21.287  2549  2589 E bt_stack: [ERROR:com_android_bluetooth_btservice_ActivityAttribution.cpp(172)] Failed to get ActivityAttribution Interface
03-12 02:01:21.687  2587  3665 E ##XLogger##: #&^HgpUYsLTpLD8oT1E5hd+TtNHONmtDi8ndVbSvVZ7tHAj/cUQ/LLrOLOqmoEs7Z75c9QC4BXijb7SX5vzx67Qmd51+lKrsH0xb/LeUuBV+amJSE8BiRaHWkOGNoiLPAw7sNx9BpZPjF8t6L/u+3+bHZLK5QQ0Q14it5VYx1Xw10I=!!WvRVoF4phNqsSr7CkzYYlEZbOJfaIyJjqyG6lRaqpNegRfnz23Pem7LLN+RWPZgzj+YlgBkuqyHfEIMpwkCNe19FL44sLADR+MJDgT5FpWAjGps9Q8MgkTZwB9/3MN7IobW11UuH+lzTt4BoQYt5JdaXK+ZlQ/I2RRRcgLP8NS1ArX5op2rYahMUBFVCKN1kzV9cEug4xainBlzhxn+KglgrqsRHFVpiShEGj0Y/VBo93ba9atS/+O8O+QIcsoHFE1fftpWyGsxuprLj6xHNgg==^&#
03-12 02:01:21.688  2587  2736 E ##XLogger##: #&^HgpUYsLTpLD8oT1E5hd+TtNHONmtDi8ndVbSvVZ7tHAj/cUQ/LLrOLOqmoEs7Z75c9QC4BXijb7SX5vzx67Qmd51+lKrsH0xb/LeUuBV+amJSE8BiRaHWkOGNoiLPAw7sNx9BpZPjF8t6L/u+3+bHZLK5QQ0Q14it5VYx1Xw10I=!!7pdXFoBZAEsfJ7u8XdBN8+XwcAKfWJO76Vm1zCBeLhiFFuD52CWrnxfPJUrKQMsWlrtZkxFqn2Pm4+iX884n8MEW6KIhgw/OfBO1wUP28OWOKYdx3W6HNM4rV77KWnxsJracch+TXVRuTPwpDPHeQg+lbwvp9/fZWqDbhgzdfKAVyd27JGQ4aYhVHNx0SKzhMoPDR3akoauYvLOv57NgdIAZfWnU3Nj3417J8kGycFZ2bUi9zPCVzpaxOoB2eIOt03kNrczXM3iMQkQBttWH47ItjuXMhubsLAT2nsvE82C/r4ubXEC5cwEviAEcOF5UKVZ6zqcp0U2eM2UCePqXwg==^&#
03-12 02:01:21.723  2587  3666 E ##XLogger##: #&^HgpUYsLTpLD8oT1E5hd+TtNHONmtDi8ndVbSvVZ7tHAj/cUQ/LLrOLOqmoEs7Z75c9QC4BXijb7SX5vzx67Qmd51+lKrsH0xb/LeUuBV+amJSE8BiRaHWkOGNoiLPAw7sNx9BpZPjF8t6L/u+3+bHZLK5QQ0Q14it5VYx1Xw10I=!!WvRVoF4phNqsSr7CkzYYlEZbOJfaIyJjqyG6lRaqpNegRfnz23Pem7LLN+RWPZgzj+YlgBkuqyHfEIMpwkCNe19FL44sLADR+MJDgT5FpWB/pkg1vyaPHDrHV6br2S7FK4l9Qid39XXsC5cKKy4PyxEINUlpcQCvR76TROTvtHibpfIedJsAfC3q8qYz7PL0XjH+4oTKo96BwSfvnMVeUQUXLPhIL3pOVb5TvfY0EPShZcB4riKKzmAA0A7eaBBq4LFiMOYGO4xvizsOT11E2w==^&#
03-12 02:01:21.724  2587  2733 E ##XLogger##: #&^HgpUYsLTpLD8oT1E5hd+TtNHONmtDi8ndVbSvVZ7tHAj/cUQ/LLrOLOqmoEs7Z75c9QC4BXijb7SX5vzx67Qmd51+lKrsH0xb/LeUuBV+amJSE8BiRaHWkOGNoiLPAw7sNx9BpZPjF8t6L/u+3+bHZLK5QQ0Q14it5VYx1Xw10I=!!7pdXFoBZAEsfJ7u8XdBN8+XwcAKfWJO76Vm1zCBeLhgTDWhuoeOP/gxcV5eNKmv4cOQFmy1kkCzHMXX2t44xk+ENTlAUZTwjTod2RmF1xoj4mu3vkJCJJX1zyLBqNCxbtcyzkod8T9i2Rl+tDs1nl47ElIPGC15AIGleJ2LWqUtCP0K9N38xLSBOUO6vxO0kFD/1JnRzzR8jScyNI+L8N7/83hNEhE7c5VjNuPdZQa7QgV29gu6lX8WT0s2uFNSkGzLQq+llCZPThcL5qmf6q4VrAyWBCo5zBgv2bIHpNQFYQ7bYB7PfCl1wltHAejPuFKVEyEfQr0GYB1qDZBYOiA==^&#
03-12 02:01:21.856  1091  1146 E HfLooper: sensor type is 4 action is 2 data is -1567.000000 1840.000000 642.000000
03-12 02:01:22.037  2587  3667 E ##XLogger##: #&^HgpUYsLTpLD8oT1E5hd+TtNHONmtDi8ndVbSvVZ7tHAj/cUQ/LLrOLOqmoEs7Z75c9QC4BXijb7SX5vzx67Qmd51+lKrsH0xb/LeUuBV+amJSE8BiRaHWkOGNoiLPAw7sNx9BpZPjF8t6L/u+3+bHZLK5QQ0Q14it5VYx1Xw10I=!!WvRVoF4phNqsSr7CkzYYlEZbOJfaIyJjqyG6lRaqpNegRfnz23Pem7LLN+RWPZgzj+YlgBkuqyHfEIMpwkCNe19FL44sLADR+MJDgT5FpWCyFfmP3sl/qfxK0riXTP8Y8064oMMpCrPmQOpemd1Wnb1gSdh53ansVp0kyAMZgq8CzTfcsAWoKb3937eyo1gVo6K87y29VCtbyAjrF/7OLtj/yOVTf0MdM6eHlj1cSywIeRsGznW4GBs4YaUP7eRn0WE/Y6jKEQ4K4id6CFJZKA==^&#
03-12 02:01:22.038  2587  2722 E ##XLogger##: #&^HgpUYsLTpLD8oT1E5hd+TtNHONmtDi8ndVbSvVZ7tHAj/cUQ/LLrOLOqmoEs7Z75c9QC4BXijb7SX5vzx67Qmd51+lKrsH0xb/LeUuBV+amJSE8BiRaHWkOGNoiLPAw7sNx9BpZPjF8t6L/u+3+bHZLK5QQ0Q14it5VYx1Xw10I=!!7pdXFoBZAEsfJ7u8XdBN8+XwcAKfWJO76Vm1zCBeLhiEKbTKtqccrUdlk/TojeTA8l4xZvhjnuRJ88niC6rqLPadFoEvtimpn1OgxtC0AnL8euJWHk6m8DdXBXs/hVeou8exAHdHsuqnzAiT6TlEo71GiJXEmF1ns4hYQj4Z6rPnQmxf1iTh6xHtX2MEyz36pMqFaBNKo+d8SQs9zYwtJtkTL1/3dYSgENQ9WxbUQxCAgcLdG776Zse2qlrLBw+S9e1FrAuTZi7GJsaOjsfrwcG4/JkHyBSwA2PLoKhOVrrO0X66JtpDVtAS5lZHaG8LBxTPaQAMdpgLFkPfoXWHuPbDSFYEAsaDveJqg43ycBzxD6i990XxvIb5BvmK5two^&#
03-12 02:01:22.223  1127  1127 E vendor.xiaomi.hardware.micharge@1.0-service: Cannot access sysfs node sys/devices/platform/soc/a600000.ssusb/super_speed
03-12 02:01:22.940  3488  3688 E NativeHotLoader: Failed to hotload library '/data/app/~~NSMZuHTkuq9YpFuPEwOGHw==/com.google.android.gms-psJCqVBg910SLXIIwhlmRg==/base.apk!/lib/arm64-v8a/libgmscore.so'. [CONTEXT service_id=447 ]
03-12 02:01:23.103  1015  1017 E vendor.xiaomi.hardware.mimd@1.0-service: success to write 262144 for device global_reclaim
03-12 02:01:23.945  3037  3400 E UsageReportingOptionsSt: INTERNAL_ERROR: can't query optInOptions while user is locked. [CONTEXT service_id=41 ]
03-12 02:01:25.791  2603  3123 E CellBroadcastUtils: getDefaultCellBroadcastReceiverPackageName: no package found
03-12 02:01:25.808  2642  2741 E OverviewComponentObserver: updateOverviewTargets: defaultHome=ComponentInfo{com.miui.home/com.miui.home.launcher.Launcher}
03-12 02:01:25.809  2642  2741 E OverviewComponentObserver: updateOverviewTargets: defaultHome=ComponentInfo{com.miui.home/com.miui.home.launcher.Launcher}
03-12 02:01:25.809  2642  2741 E OverviewComponentObserver: updateOverviewTargets: defaultHome=ComponentInfo{com.miui.home/com.miui.home.launcher.Launcher}
03-12 02:01:25.810  2642  2741 E OverviewComponentObserver: updateOverviewTargets: defaultHome=ComponentInfo{com.miui.home/com.miui.home.launcher.Launcher}
03-12 02:01:25.815  3703  3703 E ssioncontroller: Not starting debugger since process cannot load the jdwp agent.
03-12 02:01:25.834  3703  3703 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:01:25.907  3488  3695 E GservicesValue: Gservices key not allowlisted for directboot access: gms:chimera:auto_stage_all_test_apks
03-12 02:01:28.178  2603  3109 E SelfRegister.PlatformManager: subId is null
03-12 02:01:29.162  1193  1981 E HWComposer: getSupportedContentTypes: getSupportedContentTypes failed for display 0: Unsupported (8)
03-12 02:01:29.234  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.276  2448  2468 E FileManager: Fail to append log for writer is null
03-12 02:01:29.276  2448  2468 E FileManager: Fail to append log for writer is null
03-12 02:01:29.288  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.288  2448  2482 E FileManager: Fail to append log for writer is null
03-12 02:01:29.289  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.289  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.289  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.292  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.295  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.302  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.305  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.305  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.309  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.314  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.315  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.317  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.322  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.324  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.331  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.334  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.337  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.337  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.339  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.346  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.348  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.354  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.356  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.357  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.362  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.367  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.370  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.374  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.374  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.374  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.374  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.379  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.379  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.380  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.381  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.383  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.387  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.387  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.391  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.391  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.394  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.395  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.397  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.399  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.400  2626  2878 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.402  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.405  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.407  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.411  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.414  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.416  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.418  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.423  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.426  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.427  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.433  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.435  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.435  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.440  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.444  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.445  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.450  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.452  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.453  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.458  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.461  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.461  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.466  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.469  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.471  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.474  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.478  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.478  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.483  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.485  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.486  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.491  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.493  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.494  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.499  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.502  1836  1977 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.503  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.507  1449  1515 E flp     : mnld_screen_monitor_thread: Screen off
03-12 02:01:29.507  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.509  1670  2088 E libsensor_ssccalapi: sensor transfer_ssc_oem_test_msg
03-12 02:01:29.511  1670  2088 E HfManager: HfManager::configCalibration sensor = 5 err = 0
03-12 02:01:29.511  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.515  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.519  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.524  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.527  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.531  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.536  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.540  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.544  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.548  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.553  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.556  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.561  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.564  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.569  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.573  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.575  1091  1091 E SensorManager: Correct handle(5) mindelay: 200000000 old: 1000000
03-12 02:01:29.576  1091  1091 E SensorManager: Correct handle(82) mindelay: 200000000 old: 1000000
03-12 02:01:29.576  2448  2529 E FileManager: Fail to append log for writer is null
03-12 02:01:29.577  2448  2529 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.581  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.589  1193  1193 E gralloc4: Empty SMPTE 2094-40 data
03-12 02:01:29.593  1193  1193 E PerfHelper: create_cgroup mkdir err: No such file or directory
03-12 02:01:29.593  1193  1193 E PerfHelper: Failed to create cgroup
03-12 02:01:29.607  2448  2448 E FileManager: Fail to append log for writer is null
03-12 02:01:29.607  2448  2448 E FileManager: Fail to append log for writer is null
03-12 02:01:29.841  1105  1118 E mtkpower@impl: [setMode] unknown type
03-12 02:01:29.844  1091  1091 E SensorManager: Correct handle(5) mindelay: 200000000 old: 1000000
03-12 02:01:29.846  1836  1977 E libEGL  : call to OpenGL ES API with no current context (logged once per thread)
03-12 02:01:29.853  2626  3471 E KeyguardViewMediator: resetStateLocked
03-12 02:01:29.858  1578  1662 E AAL     : IOCTL_MTK_AAL_EVENTCTL error: -1
03-12 02:01:29.868  2626  2626 E LockScreenMagazinePreView: getFullScreenLayout()  mRemoteFullScreenView:null
03-12 02:01:29.868  2626  2626 E LockScreenMagazinePreView: getFullScreenLayout()  mRemoteFullScreenView:null
03-12 02:01:30.010  1091  1091 E SensorManager: Correct handle(5) mindelay: 200000000 old: 1000000
03-12 02:01:30.232  2549  2588 E bt_stack: [ERROR:com_android_bluetooth_btservice_ActivityAttribution.cpp(172)] Failed to get ActivityAttribution Interface
03-12 02:01:31.616  1015  1017 E vendor.xiaomi.hardware.mimd@1.0-service: success to write 262144 for device global_reclaim
03-12 02:01:35.263  3437  3437 E MiuiBleAppReceiver: action: miui.bluetooth.MI_TWS_HS_FEATURE_ENABLE
03-12 02:01:35.268  3437  3437 E MiuiBleAppReceiver: action: miui.bluetooth.MI_TWS_HS_FEATURE_ENABLE
03-12 02:01:35.273  3437  3437 E MiuiBleAppReceiver: action: miui.bluetooth.MI_TWS_HS_FEATURE_ENABLE
03-12 02:01:36.203  2603  3123 E CellBroadcastUtils: getDefaultCellBroadcastReceiverPackageName: no package found
03-12 02:01:39.251  1127  1178 E vendor.xiaomi.hardware.micharge@1.0-service: Failed to open node /sys/class/power_supply/wireless/reverse_chg_mode
03-12 02:01:40.701  1015  1017 E vendor.xiaomi.hardware.mimd@1.0-service: success to write 262144 for device global_reclaim
03-12 02:01:49.480  1490  1577 E WifiStandalone: conditional wait timedout
03-12 02:02:09.870  2626  3174 E ChargeUtils: cannot find the path getBatteryInfo of content://com.miui.powercenter.provider
03-12 02:02:10.019  1490  1577 E WifiStandalone: conditional wait timedout
03-12 02:02:14.197  3791  3791 E com.miui.wmsvc: Not starting debugger since process cannot load the jdwp agent.
03-12 02:02:14.294  3791  3791 E MQSEventManagerDelegate: failed to get MQSService.
03-12 02:02:17.405  2051  2051 E vendor.mediatek.hardware.aee@1.1-service: sigalrm:14 handler, aee hidl service exit.
03-12 02:02:18.878  2603  3109 E SelfRegister.PlatformManager: subId is null
03-12 02:02:18.879  2603  3109 E SelfRegister.PlatformManager: subId is null
03-12 02:02:24.260  3037  3400 E CCTFunnel: Failed find or create clearcut directory.
03-12 02:02:30.678  1490  1577 E WifiStandalone: conditional wait timedout
03-12 02:02:33.999  2603  3093 E DeviceRegister.PlatformManager: subId is null
03-12 02:02:34.009  2603  3093 E MtkPhoneIntfMgrEx: getIccAppFamily, uiccType[0] = USIMfullType = 2 iccType = 1
03-12 02:02:34.023  2603  3093 E DeviceRegister.PlatformManager: subId is null
03-12 02:02:34.037  2603  3093 E DeviceRegister.PlatformManager: subId is null
03-12 02:02:51.159  1490  1577 E WifiStandalone: conditional wait timedout
03-12 02:03:08.653  1127  1178 E vendor.xiaomi.hardware.micharge@1.0-service: Failed to open node /sys/class/power_supply/wireless/reverse_chg_mode
03-12 02:03:11.232  1490  1577 E WifiStandalone: conditional wait timedout
03-12 02:03:31.400  1490  1577 E WifiStandalone: conditional wait timedout