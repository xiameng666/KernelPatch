 520
[36476.575549][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36476.575600][ T1344] mtk_disp_c3d_config, line: 873
[36476.575608][ T1344] mtk_disp_c3d_start, line: 890
[36476.576071][ T1344] [disp_aal]Cannot find LED node from dts
[36476.576077][ T1344] [disp_aal]get pwm cust info fail
[36476.576083][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36476.576791][ T1344] [DISP]mtk_cm_config111
[36476.577025][T101344] [DISP]mtk_dsc_config+ pad_num:0
[36476.591156][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5018
[36476.592804][T726018] i2c_error_count_get 0
[36476.592840][T726018] authentic_get 1
[36476.592867][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36476.592883][T726018] charge_done_get 1
[36476.592904][T726018] capacity_raw_get 9904
[36476.592919][T726018] fastcharge_mode_set 0
[36476.592933][T726018] monitor_delay_set 30000
[36476.592952][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36476.592966][T726018] capacity_raw_get 9904
[36476.592979][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36476.592990][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36476.592998][T726018] handle_step_charge index = 3
[36476.593003][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36476.593017][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36476.593027][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36476.598610][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36476.598645][T726018] connector_temp_get 243
[36476.598657][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36476.610362][  T248] mtk_disp_tdshp_stop, line: 528
[36476.610420][  T248] mtk_disp_c3d_stop, line: 909
[36476.610474][  T248] mtk_disp_tdshp_stop, line: 528
[36476.610496][  T248] mtk_disp_c3d_stop, line: 909
[36476.611609][  T248] mtk_disp_tdshp_unprepare
[36476.611773][  T248] mtk_disp_c3d_unprepare, line: 948
[36476.611805][  T248] mtk_dmdp_aal_unprepare
[36476.612082][  T248] mtk_disp_tdshp_unprepare
[36476.612222][  T248] mtk_disp_c3d_unprepare, line: 948
[36476.612253][  T248] mtk_dmdp_aal_unprepare
[36476.615315][T103040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895]
[36476.615335][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36476.615398][T103040] [wlan][3040]kalSetPerfReport:(SW4 INFO) Rate[1300][0][0][0] RCPI[97][0][0][0]
[36476.616589][T103040] [wlan][3040]cnmTimerStartTimer:(CNM INFO) In DoTimeOut, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895] 1000 ms timercount 0
[36476.616604][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36476.616636][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36476.616683][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36476.616713][T103040] [wlan][3040]wlanLinkQualityMonitor:(SW4 INFO) Link Quality: Tx(rate:7800, total:9560, retry:1612, fail:1479, RTS fail:17, ACK fail:1462), Rx(rate:240, total:8538, dup:0, error:1970), PER(0), Congestion(idle slot:27458132, diff:10203, AwakeDur:281714)
[36476.616763][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36476.616787][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36476.616807][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36476.617183][T200254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36476.617200][T103042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[4:0:2:0:0] C:[10:20:50:80]=[6:0:0:0:0] M:[5:10:20:50]=[6:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36476.617259][T103042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23152 3211] T[2229 2226 2226 / 10779 10779 10779 10779] R[7910 / 11705] T_R[0 0 0 0 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4150/4150] txreg[7744] rxreg[16506]
[36476.618480][T103042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 000000007999dc54, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=177
[36476.618493][T200254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36476.619777][T103042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 000000001b7ae357, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=178
[36476.619802][T200254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36476.619819][ T1809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36476.619851][ T1809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36476.620035][ T1809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36476.620072][ T1809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36476.621579][ T3040] [wlan][3040]nicRxProcessEventPacket:(RX INFO) Not static config event: id=0x21, seq=177
[36476.621880][ T3040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 000000007999dc54, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=177
[36476.622592][T103040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 000000001b7ae357, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=178
[36476.623142][  T254] mtk_disp_tdshp_prepare
[36476.623990][  T254] mtk_dmdp_aal_prepare
[36476.625059][  T254] mtk_disp_tdshp_prepare
[36476.625963][T100254] mtk_dmdp_aal_prepare
[36476.627015][T100254] [cmdq] cmdq_util_enable_disp_va
[36476.627418][T100254] mtk_disp_tdshp_config, line: 411
[36476.627430][T100254] mtk_disp_tdshp_start, line: 520
[36476.627438][T100254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36476.627483][T100254] mtk_disp_c3d_config, line: 873
[36476.627494][T100254] mtk_disp_c3d_start, line: 890
[36476.628123][T100254] [disp_aal]Cannot find LED node from dts
[36476.628134][T100254] [disp_aal]get pwm cust info fail
[36476.628141][T100254] [disp_aal]disp_aal_get_cust_led mode=0
[36476.629065][T100254] [DISP]mtk_cm_config111
[36476.629113][T100254] mtk_disp_tdshp_config, line: 411
[36476.629121][T100254] mtk_disp_tdshp_start, line: 520
[36476.629128][T100254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36476.629162][T100254] mtk_disp_c3d_config, line: 873
[36476.629172][T100254] mtk_disp_c3d_start, line: 890
[36476.629647][T100254] [disp_aal]Cannot find LED node from dts
[36476.629657][T100254] [disp_aal]get pwm cust info fail
[36476.629663][T100254] [disp_aal]disp_aal_get_cust_led mode=0
[36476.630652][T100254] [DISP]mtk_cm_config111
[36476.631048][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36476.631242][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000004
[36476.704062][  T248] mtk_disp_tdshp_stop, line: 528
[36476.704136][  T248] mtk_disp_c3d_stop, line: 909
[36476.704225][  T248] mtk_disp_tdshp_stop, line: 528
[36476.704255][  T248] mtk_disp_c3d_stop, line: 909
[36476.705921][  T248] mtk_disp_tdshp_unprepare
[36476.706134][  T248] mtk_disp_c3d_unprepare, line: 948
[36476.706181][  T248] mtk_dmdp_aal_unprepare
[36476.707067][  T248] mtk_disp_tdshp_unprepare
[36476.707256][  T248] mtk_disp_c3d_unprepare, line: 948
[36476.707296][  T248] mtk_dmdp_aal_unprepare
[36476.709295][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36476.709541][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36476.709589][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36476.709653][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36476.709683][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36476.709862][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36476.709888][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36477.131783][T201344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36477.132010][T101344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36477.132085][T101344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36477.136251][ T1344] mtk_disp_tdshp_prepare
[36477.137212][ T1344] mtk_dmdp_aal_prepare
[36477.138149][ T1344] mtk_disp_tdshp_prepare
[36477.138926][ T1344] mtk_dmdp_aal_prepare
[36477.139801][ T1344] [cmdq] cmdq_util_enable_disp_va
[36477.140194][ T1344] mtk_disp_tdshp_config, line: 411
[36477.140204][ T1344] mtk_disp_tdshp_start, line: 520
[36477.140213][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.140285][ T1344] mtk_disp_c3d_config, line: 873
[36477.140295][ T1344] mtk_disp_c3d_start, line: 890
[36477.141770][ T1344] [disp_aal]Cannot find LED node from dts
[36477.141779][ T1344] [disp_aal]get pwm cust info fail
[36477.141784][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36477.142485][ T1344] [DISP]mtk_cm_config111
[36477.142538][ T1344] mtk_disp_tdshp_config, line: 411
[36477.142544][ T1344] mtk_disp_tdshp_start, line: 520
[36477.142550][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.142582][ T1344] mtk_disp_c3d_config, line: 873
[36477.142591][ T1344] mtk_disp_c3d_start, line: 890
[36477.142926][ T1344] [disp_aal]Cannot find LED node from dts
[36477.142934][ T1344] [disp_aal]get pwm cust info fail
[36477.142939][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36477.143624][ T1344] [DISP]mtk_cm_config111
[36477.143860][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36477.179127][  T248] mtk_disp_tdshp_stop, line: 528
[36477.179208][  T248] mtk_disp_c3d_stop, line: 909
[36477.179298][  T248] mtk_disp_tdshp_stop, line: 528
[36477.179328][  T248] mtk_disp_c3d_stop, line: 909
[36477.180830][  T248] mtk_disp_tdshp_unprepare
[36477.181054][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.181094][  T248] mtk_dmdp_aal_unprepare
[36477.182173][  T248] mtk_disp_tdshp_unprepare
[36477.182383][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.182425][  T248] mtk_dmdp_aal_unprepare
[36477.184532][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36477.184849][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36477.184898][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36477.184963][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36477.184994][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.185023][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.185047][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36477.185474][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36477.186016][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36477.186166][  T254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36477.188974][  T254] mtk_disp_tdshp_prepare
[36477.189904][T100254] mtk_dmdp_aal_prepare
[36477.190990][T100254] mtk_disp_tdshp_prepare
[36477.191794][T100254] mtk_dmdp_aal_prepare
[36477.192837][T100254] [cmdq] cmdq_util_enable_disp_va
[36477.193261][T100254] mtk_disp_tdshp_config, line: 411
[36477.193274][T100254] mtk_disp_tdshp_start, line: 520
[36477.193283][T100254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.193362][T100254] mtk_disp_c3d_config, line: 873
[36477.193372][T100254] mtk_disp_c3d_start, line: 890
[36477.195076][T100254] [disp_aal]Cannot find LED node from dts
[36477.195090][T100254] [disp_aal]get pwm cust info fail
[36477.195097][T100254] [disp_aal]disp_aal_get_cust_led mode=0
[36477.196022][T100254] [DISP]mtk_cm_config111
[36477.196065][T100254] mtk_disp_tdshp_config, line: 411
[36477.196072][T100254] mtk_disp_tdshp_start, line: 520
[36477.196077][T100254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.196110][T100254] mtk_disp_c3d_config, line: 873
[36477.196119][T100254] mtk_disp_c3d_start, line: 890
[36477.196505][T100254] [disp_aal]Cannot find LED node from dts
[36477.196513][T100254] [disp_aal]get pwm cust info fail
[36477.196518][T100254] [disp_aal]disp_aal_get_cust_led mode=0
[36477.197296][T100254] [DISP]mtk_cm_config111
[36477.197526][T100254] [DISP]mtk_dsc_config+ pad_num:0
[36477.234306][  T248] mtk_disp_tdshp_stop, line: 528
[36477.234386][  T248] mtk_disp_c3d_stop, line: 909
[36477.234474][  T248] mtk_disp_tdshp_stop, line: 528
[36477.234503][  T248] mtk_disp_c3d_stop, line: 909
[36477.236258][  T248] mtk_disp_tdshp_unprepare
[36477.236461][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.236501][  T248] mtk_dmdp_aal_unprepare
[36477.237339][  T248] mtk_disp_tdshp_unprepare
[36477.237524][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.237564][  T248] mtk_dmdp_aal_unprepare
[36477.239702][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36477.239949][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36477.239996][T125341] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36477.240061][T125341] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36477.240092][T125341] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.240121][T125341] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.240145][T125341] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36477.240554][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36477.240794][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36477.240919][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36477.244070][  T254] mtk_disp_tdshp_prepare
[36477.244915][  T254] mtk_dmdp_aal_prepare
[36477.246087][  T254] mtk_disp_tdshp_prepare
[36477.246958][  T254] mtk_dmdp_aal_prepare
[36477.248015][  T254] [cmdq] cmdq_util_enable_disp_va
[36477.248420][  T254] mtk_disp_tdshp_config, line: 411
[36477.248434][  T254] mtk_disp_tdshp_start, line: 520
[36477.248442][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.248522][  T254] mtk_disp_c3d_config, line: 873
[36477.248532][  T254] mtk_disp_c3d_start, line: 890
[36477.250239][  T254] [disp_aal]Cannot find LED node from dts
[36477.250260][  T254] [disp_aal]get pwm cust info fail
[36477.250267][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36477.251204][  T254] [DISP]mtk_cm_config111
[36477.251261][  T254] mtk_disp_tdshp_config, line: 411
[36477.251269][  T254] mtk_disp_tdshp_start, line: 520
[36477.251275][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.251314][  T254] mtk_disp_c3d_config, line: 873
[36477.251325][  T254] mtk_disp_c3d_start, line: 890
[36477.251767][  T254] [disp_aal]Cannot find LED node from dts
[36477.251774][  T254] [disp_aal]get pwm cust info fail
[36477.251781][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36477.252614][  T254] [DISP]mtk_cm_config111
[36477.252905][T100254] [DISP]mtk_dsc_config+ pad_num:0
[36477.287986][T300248] mtk_disp_tdshp_stop, line: 528
[36477.288057][T300248] mtk_disp_c3d_stop, line: 909
[36477.288143][T300248] mtk_disp_tdshp_stop, line: 528
[36477.288172][T300248] mtk_disp_c3d_stop, line: 909
[36477.289922][T300248] mtk_disp_tdshp_unprepare
[36477.290137][T300248] mtk_disp_c3d_unprepare, line: 948
[36477.290177][T300248] mtk_dmdp_aal_unprepare
[36477.290935][T100248] mtk_disp_tdshp_unprepare
[36477.291139][T100248] mtk_disp_c3d_unprepare, line: 948
[36477.291181][T100248] mtk_dmdp_aal_unprepare
[36477.293003][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36477.293225][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36477.293274][T25910] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36477.293337][T25910] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36477.293367][T25910] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.293396][T25910] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.293419][T25910] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36477.294264][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36477.294481][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36477.294610][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36477.297653][  T254] mtk_disp_tdshp_prepare
[36477.298526][  T254] mtk_dmdp_aal_prepare
[36477.299620][  T254] mtk_disp_tdshp_prepare
[36477.300418][  T254] mtk_dmdp_aal_prepare
[36477.301461][  T254] [cmdq] cmdq_util_enable_disp_va
[36477.301933][  T254] mtk_disp_tdshp_config, line: 411
[36477.301947][  T254] mtk_disp_tdshp_start, line: 520
[36477.301956][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.302016][  T254] mtk_disp_c3d_config, line: 873
[36477.302026][  T254] mtk_disp_c3d_start, line: 890
[36477.303488][  T254] [disp_aal]Cannot find LED node from dts
[36477.303502][  T254] [disp_aal]get pwm cust info fail
[36477.303510][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36477.304359][  T254] [DISP]mtk_cm_config111
[36477.304403][  T254] mtk_disp_tdshp_config, line: 411
[36477.304410][  T254] mtk_disp_tdshp_start, line: 520
[36477.304416][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.304447][  T254] mtk_disp_c3d_config, line: 873
[36477.304457][  T254] mtk_disp_c3d_start, line: 890
[36477.304819][  T254] [disp_aal]Cannot find LED node from dts
[36477.304826][  T254] [disp_aal]get pwm cust info fail
[36477.304832][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36477.305596][  T254] [DISP]mtk_cm_config111
[36477.305972][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36477.416090][  T248] mtk_disp_tdshp_stop, line: 528
[36477.416171][  T248] mtk_disp_c3d_stop, line: 909
[36477.416260][  T248] mtk_disp_tdshp_stop, line: 528
[36477.416290][  T248] mtk_disp_c3d_stop, line: 909
[36477.417925][  T248] mtk_disp_tdshp_unprepare
[36477.418140][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.418197][  T248] mtk_dmdp_aal_unprepare
[36477.419055][  T248] mtk_disp_tdshp_unprepare
[36477.419242][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.419283][  T248] mtk_dmdp_aal_unprepare
[36477.421367][T225960] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36477.421599][T225960] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36477.421648][T225960] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36477.421918][T225960] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36477.421980][T225960] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.422020][T225960] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.422064][T225960] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36477.615699][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5025
[36477.617177][T726018] i2c_error_count_get 0
[36477.617192][T726018] authentic_get 1
[36477.617209][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36477.617218][T726018] charge_done_get 1
[36477.617231][T726018] capacity_raw_get 9904
[36477.617243][T726018] fastcharge_mode_set 0
[36477.617251][T726018] monitor_delay_set 30000
[36477.617262][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36477.617269][T726018] capacity_raw_get 9904
[36477.617280][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36477.617289][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36477.617297][T726018] handle_step_charge index = 3
[36477.617302][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36477.617313][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36477.617323][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36477.624945][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36477.624983][T726018] connector_temp_get 243
[36477.624997][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36477.638181][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1069ms> Tput: 0(0.000mbps) [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9560:8549:49789] lv:0 th:5 fg:0x9 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:286
[36477.638252][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13097,0,0,7910,7893,0,7694,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36477.639450][T303040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895]
[36477.640650][T303040] [wlan][3040]cnmTimerStartTimer:(CNM INFO) In DoTimeOut, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895] 1000 ms timercount 0
[36477.640707][T303040] asicConnac2xFillCmdTxdInfo: 8 callbacks suppressed
[36477.640728][T303040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x85] SEQ[179] SET[0] LEN[92]
[36477.640778][T303040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x82] SEQ[180] SET[0] LEN[248]
[36477.640818][T303040] [wlan][3040]wlanLinkQualityMonitor:(SW4 INFO) Link Quality: Tx(rate:7800, total:9560, retry:1616, fail:1483, RTS fail:17, ACK fail:1466), Rx(rate:6500, total:8549, dup:0, error:1970), PER(0), Congestion(idle slot:27507921, diff:49789, AwakeDur:282202)
[36477.648055][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6966 us]
[36477.649244][ T3042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 000000007a2c558f, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=179
[36477.650285][ T3042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 000000004804dafe, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=180
[36477.650671][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:0:0:0] C:[10:20:50:80]=[0:0:0:0:0] M:[5:10:20:50]=[0:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36477.650735][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23157 3211] T[2231 2231 2229 / 10779 10779 10779 10779] R[7910 / 11708] T_R[0 0 0 2 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4150/4150] txreg[7744] rxreg[16509]
[36477.651100][T303040] [wlan][3040]nicRxProcessEventPacket:(RX INFO) Not static config event: id=0x21, seq=179
[36477.652256][T303040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 000000007a2c558f, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=179
[36477.653111][T25910] [connlog] wifi_mcu cache is full.
[36477.653152][T25910] [connlog] wifi_fw irq counter = 2793
[36477.653197][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36477.653226][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36477.658368][T303040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 000000004804dafe, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=180
[36477.675675][T103042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000004
[36477.689154][T301344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36477.689330][ T1344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36477.689415][ T1344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36477.692675][ T1344] mtk_disp_tdshp_prepare
[36477.693566][ T1344] mtk_dmdp_aal_prepare
[36477.694770][ T1344] mtk_disp_tdshp_prepare
[36477.695594][ T1344] mtk_dmdp_aal_prepare
[36477.696680][ T1344] [cmdq] cmdq_util_enable_disp_va
[36477.697172][ T1344] mtk_disp_tdshp_config, line: 411
[36477.697186][ T1344] mtk_disp_tdshp_start, line: 520
[36477.697195][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.697276][ T1344] mtk_disp_c3d_config, line: 873
[36477.697287][ T1344] mtk_disp_c3d_start, line: 890
[36477.698923][ T1344] [disp_aal]Cannot find LED node from dts
[36477.698936][ T1344] [disp_aal]get pwm cust info fail
[36477.698943][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36477.699833][ T1344] [DISP]mtk_cm_config111
[36477.699880][ T1344] mtk_disp_tdshp_config, line: 411
[36477.699887][ T1344] mtk_disp_tdshp_start, line: 520
[36477.699893][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.699928][ T1344] mtk_disp_c3d_config, line: 873
[36477.699937][ T1344] mtk_disp_c3d_start, line: 890
[36477.700350][ T1344] [disp_aal]Cannot find LED node from dts
[36477.700358][ T1344] [disp_aal]get pwm cust info fail
[36477.700363][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36477.701133][ T1344] [DISP]mtk_cm_config111
[36477.701389][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36477.702238][C500000] [name:spm&][SPM] system_bus wake up by  R12_SYSTIMER, timer_out = 318, r13 = 0x80001ec0, debug_flag = 0x10000000 0x3, r12 = 0x40000, r12_ext = 0x0, raw_sta = 0x0 0x0 0x0, idle_sta = 0x0, req_sta =  0x0 0x1f000000 0xbf0 0x3e3e001a 0x7000 0x3fd000 0x0, cg_check_sta =0xbf0, isr = 0x0, rt_req_sta0 = 0x0 rt_req_sta1 = 0x0 rt_req_sta2 = 0xffffffff rt_req_sta3 = 0xffffffff dram_sw_con_3 = 0x0, raw_ext_sta = 0x208a55, wake_misc = 0x180030, pcm_flag = 0x2890077 0x0 0x2810077 0x2810077, req = 0x6360200,  clk_settle = 0x60fe, 
[36477.739157][  T248] mtk_disp_tdshp_stop, line: 528
[36477.739260][  T248] mtk_disp_c3d_stop, line: 909
[36477.739349][  T248] mtk_disp_tdshp_stop, line: 528
[36477.739378][  T248] mtk_disp_c3d_stop, line: 909
[36477.740970][  T248] mtk_disp_tdshp_unprepare
[36477.741191][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.741232][  T248] mtk_dmdp_aal_unprepare
[36477.742125][T100248] mtk_disp_tdshp_unprepare
[36477.742335][T100248] mtk_disp_c3d_unprepare, line: 948
[36477.742398][T100248] mtk_dmdp_aal_unprepare
[36477.744710][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36477.744959][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36477.745009][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36477.745072][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36477.745103][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.745132][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.745155][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36477.745552][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36477.746172][T200254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36477.746307][T200254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36477.749185][  T254] mtk_disp_tdshp_prepare
[36477.750075][  T254] mtk_dmdp_aal_prepare
[36477.750987][  T254] mtk_disp_tdshp_prepare
[36477.751767][  T254] mtk_dmdp_aal_prepare
[36477.752656][  T254] [cmdq] cmdq_util_enable_disp_va
[36477.752991][  T254] mtk_disp_tdshp_config, line: 411
[36477.753003][  T254] mtk_disp_tdshp_start, line: 520
[36477.753011][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.753085][  T254] mtk_disp_c3d_config, line: 873
[36477.753094][  T254] mtk_disp_c3d_start, line: 890
[36477.754542][  T254] [disp_aal]Cannot find LED node from dts
[36477.754552][  T254] [disp_aal]get pwm cust info fail
[36477.754558][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36477.755416][  T254] [DISP]mtk_cm_config111
[36477.755468][  T254] mtk_disp_tdshp_config, line: 411
[36477.755474][  T254] mtk_disp_tdshp_start, line: 520
[36477.755480][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36477.755515][  T254] mtk_disp_c3d_config, line: 873
[36477.755525][  T254] mtk_disp_c3d_start, line: 890
[36477.755854][  T254] [disp_aal]Cannot find LED node from dts
[36477.755862][  T254] [disp_aal]get pwm cust info fail
[36477.755867][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36477.756636][  T254] [DISP]mtk_cm_config111
[36477.756863][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36477.863178][  T248] mtk_disp_tdshp_stop, line: 528
[36477.863259][  T248] mtk_disp_c3d_stop, line: 909
[36477.863349][  T248] mtk_disp_tdshp_stop, line: 528
[36477.863379][  T248] mtk_disp_c3d_stop, line: 909
[36477.864908][  T248] mtk_disp_tdshp_unprepare
[36477.865136][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.865176][  T248] mtk_dmdp_aal_unprepare
[36477.866231][  T248] mtk_disp_tdshp_unprepare
[36477.866433][  T248] mtk_disp_c3d_unprepare, line: 948
[36477.866474][  T248] mtk_dmdp_aal_unprepare
[36477.868329][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36477.868578][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36477.868626][T125341] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36477.868691][T125341] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36477.868722][T125341] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.868750][T125341] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36477.868775][T125341] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36477.914149][T14626] MTK-BTIF-EXP[I]mtk_wcn_btif_dpidle_ctrl:enter deep idle
[36477.967898][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7125 us]
[36478.057685][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36478.255914][T101344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36478.256111][T101344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36478.256193][T101344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36478.259051][ T1344] mtk_disp_tdshp_prepare
[36478.259937][ T1344] mtk_dmdp_aal_prepare
[36478.260930][ T1344] mtk_disp_tdshp_prepare
[36478.261853][ T1344] mtk_dmdp_aal_prepare
[36478.262817][ T1344] [cmdq] cmdq_util_enable_disp_va
[36478.263250][ T1344] mtk_disp_tdshp_config, line: 411
[36478.263263][ T1344] mtk_disp_tdshp_start, line: 520
[36478.263272][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36478.263349][ T1344] mtk_disp_c3d_config, line: 873
[36478.263360][ T1344] mtk_disp_c3d_start, line: 890
[36478.264798][ T1344] [disp_aal]Cannot find LED node from dts
[36478.264807][ T1344] [disp_aal]get pwm cust info fail
[36478.264814][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36478.265656][ T1344] [DISP]mtk_cm_config111
[36478.265797][ T1344] mtk_disp_tdshp_config, line: 411
[36478.265810][ T1344] mtk_disp_tdshp_start, line: 520
[36478.265816][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36478.265861][ T1344] mtk_disp_c3d_config, line: 873
[36478.265872][ T1344] mtk_disp_c3d_start, line: 890
[36478.266335][ T1344] [disp_aal]Cannot find LED node from dts
[36478.266343][ T1344] [disp_aal]get pwm cust info fail
[36478.266349][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36478.267144][ T1344] [DISP]mtk_cm_config111
[36478.267390][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36478.304134][T100248] mtk_disp_tdshp_stop, line: 528
[36478.304210][T100248] mtk_disp_c3d_stop, line: 909
[36478.304299][T100248] mtk_disp_tdshp_stop, line: 528
[36478.304328][T100248] mtk_disp_c3d_stop, line: 909
[36478.305189][T100248] mtk_disp_tdshp_unprepare
[36478.305312][T100248] mtk_disp_c3d_unprepare, line: 948
[36478.305329][T100248] mtk_dmdp_aal_unprepare
[36478.305522][T100248] mtk_disp_tdshp_unprepare
[36478.305583][T100248] mtk_disp_c3d_unprepare, line: 948
[36478.305597][T100248] mtk_dmdp_aal_unprepare
[36478.307306][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36478.307780][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36478.307802][T125341] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36478.307838][T125341] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36478.307849][T125341] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36478.307859][T125341] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36478.307867][T125341] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36478.308370][T400254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36478.308568][T400254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36478.308682][T400254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36478.312195][  T254] mtk_disp_tdshp_prepare
[36478.313159][  T254] mtk_dmdp_aal_prepare
[36478.314365][  T254] mtk_disp_tdshp_prepare
[36478.315330][  T254] mtk_dmdp_aal_prepare
[36478.316404][  T254] [cmdq] cmdq_util_enable_disp_va
[36478.316855][  T254] mtk_disp_tdshp_config, line: 411
[36478.316869][  T254] mtk_disp_tdshp_start, line: 520
[36478.316880][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36478.316963][  T254] mtk_disp_c3d_config, line: 873
[36478.316976][  T254] mtk_disp_c3d_start, line: 890
[36478.319104][  T254] [disp_aal]Cannot find LED node from dts
[36478.319131][  T254] [disp_aal]get pwm cust info fail
[36478.319166][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36478.320091][  T254] [DISP]mtk_cm_config111
[36478.320216][  T254] mtk_disp_tdshp_config, line: 411
[36478.320240][  T254] mtk_disp_tdshp_start, line: 520
[36478.320260][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36478.320369][  T254] mtk_disp_c3d_config, line: 873
[36478.320400][  T254] mtk_disp_c3d_start, line: 890
[36478.321563][  T254] [disp_aal]Cannot find LED node from dts
[36478.321585][  T254] [disp_aal]get pwm cust info fail
[36478.321604][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36478.322575][  T254] [DISP]mtk_cm_config111
[36478.323100][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36478.432518][T100248] mtk_disp_tdshp_stop, line: 528
[36478.432591][T100248] mtk_disp_c3d_stop, line: 909
[36478.432670][T100248] mtk_disp_tdshp_stop, line: 528
[36478.432695][T100248] mtk_disp_c3d_stop, line: 909
[36478.434332][  T248] mtk_disp_tdshp_unprepare
[36478.434523][  T248] mtk_disp_c3d_unprepare, line: 948
[36478.434557][  T248] mtk_dmdp_aal_unprepare
[36478.435306][  T248] mtk_disp_tdshp_unprepare
[36478.435459][  T248] mtk_disp_c3d_unprepare, line: 948
[36478.435492][  T248] mtk_dmdp_aal_unprepare
[36478.437235][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36478.437359][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36478.437379][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36478.437415][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36478.437426][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36478.437437][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36478.437446][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36478.512317][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[2237 us]
[36478.541373][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000001
[36478.641472][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5043
[36478.643609][T726018] i2c_error_count_get 0
[36478.643638][T726018] authentic_get 1
[36478.643657][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36478.643668][T726018] charge_done_get 1
[36478.643684][T726018] capacity_raw_get 9904
[36478.643695][T726018] fastcharge_mode_set 0
[36478.643705][T726018] monitor_delay_set 30000
[36478.643719][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36478.643727][T726018] capacity_raw_get 9904
[36478.643738][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36478.643750][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36478.643758][T726018] handle_step_charge index = 3
[36478.643763][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36478.643776][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36478.643785][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36478.650681][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36478.650715][T726018] connector_temp_get 243
[36478.650725][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36478.664003][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1026ms> Tput: 1968(0.001mbps) [74:1:179:1][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9574:8549:2157] lv:0 th:5 fg:0x9 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:286
[36478.664106][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13103,0,0,7911,7893,0,7694,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36478.665650][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895]
[36478.665914][ T3040] [wlan][3040]kalSetPerfReport:(SW4 INFO) Rate[48][0][0][0] RCPI[111][0][0][0]
[36478.665974][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x7E] SEQ[181] SET[1] LEN[376]
[36478.667483][ T3040] [wlan][3040]cnmTimerStartTimer:(CNM INFO) In DoTimeOut, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895] 1000 ms timercount 0
[36478.667790][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x85] SEQ[182] SET[0] LEN[92]
[36478.667856][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x82] SEQ[183] SET[0] LEN[248]
[36478.667907][ T3040] [wlan][3040]wlanLinkQualityMonitor:(SW4 INFO) Link Quality: Tx(rate:7800, total:9574, retry:1616, fail:1483, RTS fail:17, ACK fail:1466), Rx(rate:6500, total:8549, dup:0, error:1970), PER(0), Congestion(idle slot:27510078, diff:2157, AwakeDur:282227)
[36478.673918][T25910] sd 0:0:0:2: [sdc] Synchronizing SCSI cache
[36478.674966][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6782 us]
[36478.676551][ T3042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000afd13989, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=182
[36478.678041][ T3042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 0000000046896cf1, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=183
[36478.678915][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:1:0:0] C:[10:20:50:80]=[1:0:0:0:0] M:[5:10:20:50]=[1:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36478.679008][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23165 3212] T[2234 2234 2231 / 10780 10780 10780 10780] R[7911 / 11714] T_R[0 0 0 3 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4150/4150] txreg[7745] rxreg[16516]
[36478.681305][T203040] [wlan][3040]nicRxProcessEventPacket:(RX INFO) Not static config event: id=0x21, seq=182
[36478.682792][T203040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000afd13989, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=182
[36478.682813][ T1809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36478.684562][T203040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 0000000046896cf1, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=183
[36478.684598][ T1809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36478.703263][T103042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000004
[36478.824115][T201344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36478.824312][ T1344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36478.824399][ T1344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36478.827728][ T1344] mtk_disp_tdshp_prepare
[36478.828618][ T1344] mtk_dmdp_aal_prepare
[36478.829884][ T1344] mtk_disp_tdshp_prepare
[36478.830705][ T1344] mtk_dmdp_aal_prepare
[36478.831792][ T1344] [cmdq] cmdq_util_enable_disp_va
[36478.832220][ T1344] mtk_disp_tdshp_config, line: 411
[36478.832234][ T1344] mtk_disp_tdshp_start, line: 520
[36478.832243][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36478.832325][ T1344] mtk_disp_c3d_config, line: 873
[36478.832337][ T1344] mtk_disp_c3d_start, line: 890
[36478.834083][ T1344] [disp_aal]Cannot find LED node from dts
[36478.834098][ T1344] [disp_aal]get pwm cust info fail
[36478.834105][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36478.835074][ T1344] [DISP]mtk_cm_config111
[36478.835135][ T1344] mtk_disp_tdshp_config, line: 411
[36478.835143][ T1344] mtk_disp_tdshp_start, line: 520
[36478.835150][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36478.835190][ T1344] mtk_disp_c3d_config, line: 873
[36478.835201][ T1344] mtk_disp_c3d_start, line: 890
[36478.835686][ T1344] [disp_aal]Cannot find LED node from dts
[36478.835694][ T1344] [disp_aal]get pwm cust info fail
[36478.835700][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36478.836491][ T1344] [DISP]mtk_cm_config111
[36478.836729][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36478.944168][  T248] mtk_disp_tdshp_stop, line: 528
[36478.944245][  T248] mtk_disp_c3d_stop, line: 909
[36478.944334][  T248] mtk_disp_tdshp_stop, line: 528
[36478.944364][  T248] mtk_disp_c3d_stop, line: 909
[36478.946120][  T248] mtk_disp_tdshp_unprepare
[36478.946342][  T248] mtk_disp_c3d_unprepare, line: 948
[36478.946383][  T248] mtk_dmdp_aal_unprepare
[36478.946731][  T248] mtk_disp_tdshp_unprepare
[36478.946902][  T248] mtk_disp_c3d_unprepare, line: 948
[36478.946941][  T248] mtk_dmdp_aal_unprepare
[36478.948756][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36478.948984][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36478.949030][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36478.949094][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36478.949124][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36478.949152][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36478.949175][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36478.982406][T100421] [ccci1/fsm]poll MD status send msg 0
[36478.992825][T114508] [ccci1/fsm]received MD status response 896e0043
[36478.993264][T300421] [ccci1/fsm]poll MD status wait done 3748
[36479.079924][T303040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x84] SEQ[184] SET[0] LEN[72]
[36479.088137][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7794 us]
[36479.089381][ T3042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000d877b1cf, nicCmdEventQueryLinkStats.cfi_jt [wlan_drv_gen4m_6895], cmd=0x84, seq=184
[36479.096443][T303040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000d877b1cf, nicCmdEventQueryLinkStats.cfi_jt [wlan_drv_gen4m_6895], cmd=0x84, seq=184
[36479.104849][T103040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x81] SEQ[185] SET[0] LEN[64]
[36479.106413][T303042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000a6a313db, nicCmdEventQueryLinkSpeedEx.cfi_jt [wlan_drv_gen4m_6895], cmd=0x81, seq=185
[36479.111873][T103040] [wlan][3040]nicUpdateLinkQuality:(RLM INFO) Rssi=-51, NewRssi=-51
[36479.113066][T103040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000a6a313db, nicCmdEventQueryLinkSpeedEx.cfi_jt [wlan_drv_gen4m_6895], cmd=0x81, seq=185
[36479.113091][T103040] [wlan][3040]soc7_0_get_rx_rate_info:(SW4 ERROR) prStaRecOfAP is null
[36479.114790][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000000
[36479.120544][T301462] [wlan][1462]mtk_cfg80211_get_station:(REQ INFO) link speed=8667/8667, bw=0/0, rssi=-51, BSSID:[04:67:**:**:**:10],TxFail=0, TxTimeOut=0, TxOK=10780, RxOK=7911, FcsErr=0
[36479.390776][T201344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36479.391197][T301344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36479.391277][T301344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36479.395431][ T1344] mtk_disp_tdshp_prepare
[36479.396389][ T1344] mtk_dmdp_aal_prepare
[36479.397257][ T1344] mtk_disp_tdshp_prepare
[36479.398088][ T1344] mtk_dmdp_aal_prepare
[36479.398924][ T1344] [cmdq] cmdq_util_enable_disp_va
[36479.399301][ T1344] mtk_disp_tdshp_config, line: 411
[36479.399312][ T1344] mtk_disp_tdshp_start, line: 520
[36479.399320][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36479.399395][ T1344] mtk_disp_c3d_config, line: 873
[36479.399404][ T1344] mtk_disp_c3d_start, line: 890
[36479.400733][ T1344] [disp_aal]Cannot find LED node from dts
[36479.400741][ T1344] [disp_aal]get pwm cust info fail
[36479.400747][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36479.401443][ T1344] [DISP]mtk_cm_config111
[36479.401497][ T1344] mtk_disp_tdshp_config, line: 411
[36479.401504][ T1344] mtk_disp_tdshp_start, line: 520
[36479.401510][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36479.401545][ T1344] mtk_disp_c3d_config, line: 873
[36479.401553][ T1344] mtk_disp_c3d_start, line: 890
[36479.401891][ T1344] [disp_aal]Cannot find LED node from dts
[36479.401899][ T1344] [disp_aal]get pwm cust info fail
[36479.401905][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36479.402582][ T1344] [DISP]mtk_cm_config111
[36479.402810][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36479.510378][T100248] mtk_disp_tdshp_stop, line: 528
[36479.510455][T100248] mtk_disp_c3d_stop, line: 909
[36479.510543][T100248] mtk_disp_tdshp_stop, line: 528
[36479.510572][T100248] mtk_disp_c3d_stop, line: 909
[36479.511594][T100248] mtk_disp_tdshp_unprepare
[36479.511692][T100248] mtk_disp_c3d_unprepare, line: 948
[36479.511707][T100248] mtk_dmdp_aal_unprepare
[36479.511889][T100248] mtk_disp_tdshp_unprepare
[36479.511948][T100248] mtk_disp_c3d_unprepare, line: 948
[36479.511961][T100248] mtk_dmdp_aal_unprepare
[36479.513404][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36479.513670][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36479.513857][T25910] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36479.513930][T25910] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36479.513956][T25910] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36479.513991][T25910] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36479.514029][T25910] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36479.666454][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5043
[36479.668790][T726018] i2c_error_count_get 0
[36479.668827][T726018] authentic_get 1
[36479.668854][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36479.668870][T726018] charge_done_get 1
[36479.668891][T726018] capacity_raw_get 9904
[36479.668907][T726018] fastcharge_mode_set 0
[36479.668921][T726018] monitor_delay_set 30000
[36479.668941][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36479.668955][T726018] capacity_raw_get 9904
[36479.668969][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36479.668981][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36479.668989][T726018] handle_step_charge index = 3
[36479.668994][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36479.669007][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36479.669017][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36479.674634][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36479.674667][T726018] connector_temp_get 243
[36479.674678][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36479.686102][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1022ms> Tput: 0(0.000mbps) [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9574:8550:11476] lv:0 th:5 fg:0x9 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:286
[36479.686205][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13109,0,0,7911,7893,0,7694,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36479.687754][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895]
[36479.689386][ T3040] [wlan][3040]cnmTimerStartTimer:(CNM INFO) In DoTimeOut, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895] 1000 ms timercount 0
[36479.689473][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x85] SEQ[186] SET[0] LEN[92]
[36479.689520][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x82] SEQ[187] SET[0] LEN[248]
[36479.689570][ T3040] [wlan][3040]wlanLinkQualityMonitor:(SW4 INFO) Link Quality: Tx(rate:7020, total:9574, retry:1616, fail:1483, RTS fail:17, ACK fail:1466), Rx(rate:240, total:8550, dup:0, error:1970), PER(0), Congestion(idle slot:27521554, diff:11476, AwakeDur:282353)
[36479.697833][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7871 us]
[36479.699407][ T3042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000a1216c01, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=186
[36479.701027][T25910] [connlog] wifi_fw irq counter = 2795
[36479.702629][ T3042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000c99558a5, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=187
[36479.702648][T101809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36479.702838][T101809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36479.703179][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:0:0:0] C:[10:20:50:80]=[0:0:0:0:0] M:[5:10:20:50]=[0:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36479.703249][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23172 3214] T[2238 2238 2236 / 10780 10780 10780 10780] R[7911 / 11720] T_R[0 0 0 2 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4150/4150] txreg[7745] rxreg[16522]
[36479.703757][T203040] [wlan][3040]nicRxProcessEventPacket:(RX INFO) Not static config event: id=0x21, seq=186
[36479.705248][T203040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000a1216c01, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=186
[36479.706771][T203040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000c99558a5, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=187
[36479.726220][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000004
[36479.926035][T200159] [wdk-c] cpu=2 o_k=2 lbit=0x4 cbit=0xff,fb,7,1,773528308,ff,0,0,0,0,[36479925995199,15000000] 26
[36479.926042][T600163] [wdk-c] cpu=6 o_k=6 lbit=0x44 cbit=0xff,bb,7,1,773528308,ff,0,0,0,0,[36479926018969,14999975] 26
[36479.926242][T300160] [wdk-c] cpu=3 o_k=3 lbit=0x4c cbit=0xff,b3,7,1,773528308,ff,0,0,0,0,[36479926153738,14999841] 26
[36479.942015][T700164] [wdk-c] cpu=7 o_k=7 lbit=0xcc cbit=0xff,33,7,1,773528308,ff,0,0,0,0,[36479941990353,14984004] 26
[36479.954219][T400161] [wdk-c] cpu=4 o_k=4 lbit=0xdc cbit=0xff,23,7,1,773528308,ff,0,0,0,0,[36479954163584,14971832] 26
[36479.958031][T500162] [wdk-c] cpu=5 o_k=5 lbit=0xfc cbit=0xff,3,7,1,773528308,ff,0,0,0,0,[36479957999507,14967995] 26
[36479.964989][T201344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36479.965144][ T1344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36479.965218][ T1344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36479.968199][ T1344] mtk_disp_tdshp_prepare
[36479.969060][ T1344] mtk_dmdp_aal_prepare
[36479.969944][ T1344] mtk_disp_tdshp_prepare
[36479.970734][ T1344] mtk_dmdp_aal_prepare
[36479.971559][ T1344] [cmdq] cmdq_util_enable_disp_va
[36479.971905][ T1344] mtk_disp_tdshp_config, line: 411
[36479.971915][ T1344] mtk_disp_tdshp_start, line: 520
[36479.971924][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36479.971996][ T1344] mtk_disp_c3d_config, line: 873
[36479.972006][ T1344] mtk_disp_c3d_start, line: 890
[36479.973356][ T1344] [disp_aal]Cannot find LED node from dts
[36479.973363][ T1344] [disp_aal]get pwm cust info fail
[36479.973369][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36479.974116][ T1344] [DISP]mtk_cm_config111
[36479.974177][ T1344] mtk_disp_tdshp_config, line: 411
[36479.974184][ T1344] mtk_disp_tdshp_start, line: 520
[36479.974190][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36479.974233][ T1344] mtk_disp_c3d_config, line: 873
[36479.974241][ T1344] mtk_disp_c3d_start, line: 890
[36479.974630][ T1344] [disp_aal]Cannot find LED node from dts
[36479.974636][ T1344] [disp_aal]get pwm cust info fail
[36479.974642][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36479.975328][ T1344] [DISP]mtk_cm_config111
[36479.975564][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36479.998269][T100158] [wdk-c] cpu=1 o_k=1 lbit=0xfe cbit=0xff,1,7,1,773528308,ff,0,0,0,0,[36479998198584,14927798] 26
[36480.001959][  T157] [thread:157] 2026-03-12 05:01:07.116231 UTC;android time 2026-03-12 13:01:07.116231
[36480.001999][  T157] [wdk-k] cpu=0 o_k=0 lbit=0xff cbit=0xff,0,7,1,773528308,ff,0,0,0,0,[36480001898353,14924097] 26
[36480.046797][  T248] mtk_disp_tdshp_stop, line: 528
[36480.046877][  T248] mtk_disp_c3d_stop, line: 909
[36480.046965][  T248] mtk_disp_tdshp_stop, line: 528
[36480.046994][  T248] mtk_disp_c3d_stop, line: 909
[36480.048495][  T248] mtk_disp_tdshp_unprepare
[36480.048706][  T248] mtk_disp_c3d_unprepare, line: 948
[36480.048746][  T248] mtk_dmdp_aal_unprepare
[36480.049638][  T248] mtk_disp_tdshp_unprepare
[36480.050007][  T248] mtk_disp_c3d_unprepare, line: 948
[36480.050073][  T248] mtk_dmdp_aal_unprepare
[36480.052087][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36480.052492][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36480.052541][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36480.052608][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36480.052638][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36480.052668][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36480.052692][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36480.521856][T25910] timesync host boottime 36480480158173
[36480.531413][T201344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36480.531625][ T1344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36480.531690][ T1344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36480.534398][ T1344] mtk_disp_tdshp_prepare
[36480.535208][ T1344] mtk_dmdp_aal_prepare
[36480.536062][ T1344] mtk_disp_tdshp_prepare
[36480.536836][ T1344] mtk_dmdp_aal_prepare
[36480.537664][ T1344] [cmdq] cmdq_util_enable_disp_va
[36480.538090][ T1344] mtk_disp_tdshp_config, line: 411
[36480.538100][ T1344] mtk_disp_tdshp_start, line: 520
[36480.538106][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36480.538151][ T1344] mtk_disp_c3d_config, line: 873
[36480.538160][ T1344] mtk_disp_c3d_start, line: 890
[36480.539093][ T1344] [disp_aal]Cannot find LED node from dts
[36480.539100][ T1344] [disp_aal]get pwm cust info fail
[36480.539105][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36480.539799][ T1344] [DISP]mtk_cm_config111
[36480.539846][ T1344] mtk_disp_tdshp_config, line: 411
[36480.539853][ T1344] mtk_disp_tdshp_start, line: 520
[36480.539859][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36480.539893][ T1344] mtk_disp_c3d_config, line: 873
[36480.539901][ T1344] mtk_disp_c3d_start, line: 890
[36480.540273][ T1344] [disp_aal]Cannot find LED node from dts
[36480.540279][ T1344] [disp_aal]get pwm cust info fail
[36480.540285][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36480.540958][ T1344] [DISP]mtk_cm_config111
[36480.541179][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36480.614270][  T248] mtk_disp_tdshp_stop, line: 528
[36480.614340][  T248] mtk_disp_c3d_stop, line: 909
[36480.614417][  T248] mtk_disp_tdshp_stop, line: 528
[36480.614441][  T248] mtk_disp_c3d_stop, line: 909
[36480.615714][  T248] mtk_disp_tdshp_unprepare
[36480.615888][  T248] mtk_disp_c3d_unprepare, line: 948
[36480.615922][  T248] mtk_dmdp_aal_unprepare
[36480.616588][  T248] mtk_disp_tdshp_unprepare
[36480.616737][  T248] mtk_disp_c3d_unprepare, line: 948
[36480.616769][  T248] mtk_dmdp_aal_unprepare
[36480.618442][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36480.618631][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36480.618673][T25910] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36480.618730][T25910] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36480.618755][T25910] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36480.618779][T25910] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36480.618799][T25910] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36480.688342][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5043
[36480.690085][T726018] i2c_error_count_get 0
[36480.690117][T726018] authentic_get 1
[36480.690143][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36480.690158][T726018] charge_done_get 1
[36480.690177][T726018] capacity_raw_get 9904
[36480.690191][T726018] fastcharge_mode_set 0
[36480.690200][T726018] monitor_delay_set 30000
[36480.690213][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36480.690220][T726018] capacity_raw_get 9904
[36480.690231][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36480.690241][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36480.690249][T726018] handle_step_charge index = 3
[36480.690254][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36480.690266][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36480.690276][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36480.699397][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36480.699439][T726018] connector_temp_get 243
[36480.699452][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36480.710195][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1024ms> Tput: 0(0.000mbps) [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9574:8550:2317] lv:0 th:5 fg:0x9 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:286
[36480.710285][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13112,0,0,7911,7893,0,7694,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36480.711845][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895]
[36480.713361][ T3040] [wlan][3040]cnmTimerStartTimer:(CNM INFO) In DoTimeOut, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895] 1000 ms timercount 0
[36480.713443][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x85] SEQ[188] SET[0] LEN[92]
[36480.713502][ T3040] [wlan][3040]wlanLinkQualityMonitor:(SW4 INFO) Link Quality: Tx(rate:7020, total:9574, retry:1616, fail:1483, RTS fail:17, ACK fail:1466), Rx(rate:240, total:8550, dup:0, error:1970), PER(0), Congestion(idle slot:27523871, diff:2317, AwakeDur:282399)
[36480.722170][T103042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[8249 us]
[36480.723702][T103042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 000000003fc9edb1, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=188
[36480.723724][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36480.723753][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36480.725272][T103042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 0000000083021703, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=189
[36480.725644][T103042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:0:0:0] C:[10:20:50:80]=[0:0:0:0:0] M:[5:10:20:50]=[0:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36480.725934][T103042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23175 3215] T[2240 2240 2238 / 10780 10780 10780 10780] R[7911 / 11723] T_R[0 0 0 2 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4150/4150] txreg[7745] rxreg[16525]
[36480.726816][T103040] [wlan][3040]nicRxProcessEventPacket:(RX INFO) Not static config event: id=0x21, seq=188
[36480.728309][T103040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 000000003fc9edb1, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=188
[36480.729833][T103040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 0000000083021703, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=189
[36480.748805][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000000
[36481.073128][T526702] [wlan][wlan][26702]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0x954c] TransID[0x1bb4] SeqNo[158]
[36481.074796][T626703] [wlan][wlan][26703]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0x5d75] TransID[0x10e8] SeqNo[159]
[36481.075090][T126704] [wlan][wlan][26704]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0x6828] TransID[0x71ae] SeqNo[160]
[36481.075687][T626705] [wlan][wlan][26705]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0x3480] TransID[0xc32f] SeqNo[161]
[36481.077174][T626708] [wlan][wlan][26708]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0xe917] TransID[0x0005] SeqNo[162]
[36481.077389][T626710] [wlan][wlan][26710]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0xecec] TransID[0x274d] SeqNo[163]
[36481.080949][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7547 us]
[36481.085976][T303040] [wlan][wlan][3040]wlanPktTxDone:(TX INFO) TX DONE, Type[DNS] Tag[0x6c619300] WIDX:PID[6:1] Status[0], SeqNo: 158
[36481.097383][T303040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x733a, TransID 0x1bb4
[36481.097551][T303040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x733b, TransID 0x10e8
[36481.102657][ T1344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36481.102856][T301344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36481.102918][T301344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36481.105624][ T1344] mtk_disp_tdshp_prepare
[36481.106496][ T1344] mtk_dmdp_aal_prepare
[36481.107343][ T1344] mtk_disp_tdshp_prepare
[36481.108104][ T1344] mtk_dmdp_aal_prepare
[36481.108921][ T1344] [cmdq] cmdq_util_enable_disp_va
[36481.109253][ T1344] mtk_disp_tdshp_config, line: 411
[36481.109262][ T1344] mtk_disp_tdshp_start, line: 520
[36481.109269][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36481.109322][ T1344] mtk_disp_c3d_config, line: 873
[36481.109333][ T1344] mtk_disp_c3d_start, line: 890
[36481.110519][ T1344] [disp_aal]Cannot find LED node from dts
[36481.110527][ T1344] [disp_aal]get pwm cust info fail
[36481.110534][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36481.111232][ T1344] [DISP]mtk_cm_config111
[36481.111280][ T1344] mtk_disp_tdshp_config, line: 411
[36481.111286][ T1344] mtk_disp_tdshp_start, line: 520
[36481.111292][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36481.111328][ T1344] mtk_disp_c3d_config, line: 873
[36481.111337][ T1344] mtk_disp_c3d_start, line: 890
[36481.111732][ T1344] [disp_aal]Cannot find LED node from dts
[36481.111739][ T1344] [disp_aal]get pwm cust info fail
[36481.111745][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36481.112424][ T1344] [DISP]mtk_cm_config111
[36481.112636][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36481.118598][T203040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x733c, TransID 0x71ae
[36481.127498][T303040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x733d, TransID 0xc32f
[36481.127681][T303040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x733e, TransID 0x0005
[36481.128278][T303040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x733f, TransID 0x274d
[36481.146161][  T248] mtk_disp_tdshp_stop, line: 528
[36481.146203][  T248] mtk_disp_c3d_stop, line: 909
[36481.146247][  T248] mtk_disp_tdshp_stop, line: 528
[36481.146260][  T248] mtk_disp_c3d_stop, line: 909
[36481.147152][  T248] mtk_disp_tdshp_unprepare
[36481.147253][  T248] mtk_disp_c3d_unprepare, line: 948
[36481.147272][  T248] mtk_dmdp_aal_unprepare
[36481.147461][  T248] mtk_disp_tdshp_unprepare
[36481.147535][  T248] mtk_disp_c3d_unprepare, line: 948
[36481.147553][  T248] mtk_dmdp_aal_unprepare
[36481.148800][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36481.148977][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36481.149000][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36481.149035][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36481.149048][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36481.149059][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36481.149069][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36481.151696][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36481.152309][T400254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36481.152423][T400254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36481.155651][T100254] mtk_disp_tdshp_prepare
[36481.156495][  T254] mtk_dmdp_aal_prepare
[36481.157384][  T254] mtk_disp_tdshp_prepare
[36481.158160][  T254] mtk_dmdp_aal_prepare
[36481.158992][  T254] [cmdq] cmdq_util_enable_disp_va
[36481.159337][  T254] mtk_disp_tdshp_config, line: 411
[36481.159347][  T254] mtk_disp_tdshp_start, line: 520
[36481.159355][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36481.159407][  T254] mtk_disp_c3d_config, line: 873
[36481.159416][  T254] mtk_disp_c3d_start, line: 890
[36481.160612][  T254] [disp_aal]Cannot find LED node from dts
[36481.160618][  T254] [disp_aal]get pwm cust info fail
[36481.160625][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36481.161322][  T254] [DISP]mtk_cm_config111
[36481.161371][  T254] mtk_disp_tdshp_config, line: 411
[36481.161379][  T254] mtk_disp_tdshp_start, line: 520
[36481.161385][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36481.161425][  T254] mtk_disp_c3d_config, line: 873
[36481.161433][  T254] mtk_disp_c3d_start, line: 890
[36481.162070][  T254] [disp_aal]Cannot find LED node from dts
[36481.162078][  T254] [disp_aal]get pwm cust info fail
[36481.162084][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36481.162771][  T254] [DISP]mtk_cm_config111
[36481.163013][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36481.198055][T100248] mtk_disp_tdshp_stop, line: 528
[36481.198109][T100248] mtk_disp_c3d_stop, line: 909
[36481.198176][T100248] mtk_disp_tdshp_stop, line: 528
[36481.198198][T100248] mtk_disp_c3d_stop, line: 909
[36481.199421][T100248] mtk_disp_tdshp_unprepare
[36481.199582][T100248] mtk_disp_c3d_unprepare, line: 948
[36481.199613][T100248] mtk_dmdp_aal_unprepare
[36481.199877][T100248] mtk_disp_tdshp_unprepare
[36481.200008][T100248] mtk_disp_c3d_unprepare, line: 948
[36481.200039][T100248] mtk_dmdp_aal_unprepare
[36481.201610][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36481.201899][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36481.201965][T125341] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36481.202031][T125341] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36481.202075][T125341] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36481.202111][T125341] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36481.202151][T125341] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36481.202820][T200254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36481.203171][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36481.203324][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36481.206144][  T254] mtk_disp_tdshp_prepare
[36481.206959][  T254] mtk_dmdp_aal_prepare
[36481.207892][  T254] mtk_disp_tdshp_prepare
[36481.208670][  T254] mtk_dmdp_aal_prepare
[36481.209620][  T254] [cmdq] cmdq_util_enable_disp_va
[36481.210044][  T254] mtk_disp_tdshp_config, line: 411
[36481.210056][  T254] mtk_disp_tdshp_start, line: 520
[36481.210063][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36481.210139][  T254] mtk_disp_c3d_config, line: 873
[36481.210150][  T254] mtk_disp_c3d_start, line: 890
[36481.211358][  T254] [disp_aal]Cannot find LED node from dts
[36481.211365][  T254] [disp_aal]get pwm cust info fail
[36481.211371][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36481.212168][  T254] [DISP]mtk_cm_config111
[36481.212217][  T254] mtk_disp_tdshp_config, line: 411
[36481.212224][  T254] mtk_disp_tdshp_start, line: 520
[36481.212230][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36481.212270][  T254] mtk_disp_c3d_config, line: 873
[36481.212279][  T254] mtk_disp_c3d_start, line: 890
[36481.212757][  T254] [disp_aal]Cannot find LED node from dts
[36481.212764][  T254] [disp_aal]get pwm cust info fail
[36481.212769][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36481.213500][  T254] [DISP]mtk_cm_config111
[36481.213841][T100254] [DISP]mtk_dsc_config+ pad_num:0
[36481.282587][T303040] <FW>PWR SINFO-1: 2 ce 2 8 22c760c ef 0 41531 0 d7 0 4442ad 0 0 0 dca19cf 0 22c6922 58 0 0 0 0 0 50 0 0 0 0 0 0 8 0 0 0 0 0 22c61ac 98 0 0 0 0 0 92 0 0 0 0 0 0 6 0 0 0 0 0 22c00dd 752f 0 0 0 0 0 b95 0 0 0 0 0 0 4b6 0 0 11a 0 63ca 9a 28d 4a 227 4a 1cf 6 30 e9
[36481.283796][T303040] <FW>PWR SINFO-2: 33f 0 0 0 0 0 0 0 0 47 49 0 4 4 0 0 0 0 f0534eb2 22c0128 f0559b8c 22c5ee3 f0534eb2 22c6124 f0534eb2 22c61ac 0 0 0 0 0 0 0 0 f0534eb2 3 f0559b8c 1 0 0 0 0 0 0 0 0 0 0 0 0 f 0 0 0 0 0 0 0 0 49 3d 0 7e 18 0 0 0 0 0 0 0 0 a81 37 0 0 0 0 0 0 fe 0
[36481.284052][T303040] <FW>PWR SINFO-3: ac8 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 110 c 0 0 40 0 0 10000 91010064 10000 10000 1 ef 62 56 0 0 0 0 0
[36481.284916][ T3040] <FW>PWR RATIO-1: 2 30003 25258 4547 10 188 5000000 2 4 8 29 2
[36481.290064][  T248] mtk_disp_tdshp_stop, line: 528
[36481.290101][  T248] mtk_disp_c3d_stop, line: 909
[36481.290165][  T248] mtk_disp_tdshp_stop, line: 528
[36481.290182][  T248] mtk_disp_c3d_stop, line: 909
[36481.291228][  T248] mtk_disp_tdshp_unprepare
[36481.291497][  T248] mtk_disp_c3d_unprepare, line: 948
[36481.291525][  T248] mtk_dmdp_aal_unprepare
[36481.291778][  T248] mtk_disp_tdshp_unprepare
[36481.291899][  T248] mtk_disp_c3d_unprepare, line: 948
[36481.291922][  T248] mtk_dmdp_aal_unprepare
[36481.293370][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36481.293523][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36481.293552][T25910] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36481.293594][T25910] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36481.293611][T25910] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36481.293626][T25910] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36481.293639][T25910] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36481.436390][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36481.482931][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[2506 us]
[36481.545381][T103042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000004
[36481.586147][T103042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[5667 us]
[36481.613105][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000001
[36481.661669][T201344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36481.661877][ T1344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36481.661955][ T1344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36481.665050][ T1344] mtk_disp_tdshp_prepare
[36481.665923][ T1344] mtk_dmdp_aal_prepare
[36481.666922][ T1344] mtk_disp_tdshp_prepare
[36481.667720][ T1344] mtk_dmdp_aal_prepare
[36481.668678][ T1344] [cmdq] cmdq_util_enable_disp_va
[36481.669097][ T1344] mtk_disp_tdshp_config, line: 411
[36481.669109][ T1344] mtk_disp_tdshp_start, line: 520
[36481.669117][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36481.669193][ T1344] mtk_disp_c3d_config, line: 873
[36481.669203][ T1344] mtk_disp_c3d_start, line: 890
[36481.670747][ T1344] [disp_aal]Cannot find LED node from dts
[36481.670758][ T1344] [disp_aal]get pwm cust info fail
[36481.670764][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36481.671572][ T1344] [DISP]mtk_cm_config111
[36481.671624][ T1344] mtk_disp_tdshp_config, line: 411
[36481.671631][ T1344] mtk_disp_tdshp_start, line: 520
[36481.671637][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36481.671677][ T1344] mtk_disp_c3d_config, line: 873
[36481.671686][ T1344] mtk_disp_c3d_start, line: 890
[36481.672133][ T1344] [disp_aal]Cannot find LED node from dts
[36481.672140][ T1344] [disp_aal]get pwm cust info fail
[36481.672146][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36481.672917][ T1344] [DISP]mtk_cm_config111
[36481.673170][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36481.711510][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5043
[36481.713388][T726018] i2c_error_count_get 0
[36481.713425][T726018] authentic_get 1
[36481.713453][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36481.713469][T726018] charge_done_get 1
[36481.713489][T726018] capacity_raw_get 9904
[36481.713505][T726018] fastcharge_mode_set 0
[36481.713518][T726018] monitor_delay_set 30000
[36481.713537][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36481.713551][T726018] capacity_raw_get 9904
[36481.713564][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36481.713575][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36481.713583][T726018] handle_step_charge index = 3
[36481.713588][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36481.713601][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36481.713610][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36481.721679][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36481.721749][T726018] connector_temp_get 243
[36481.721763][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36481.733859][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1024ms> Tput: 117368(0.114mbps) [4680:40:10344:40][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9574:8550:1351] lv:0 th:5 fg:0x9 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:291
[36481.733935][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13164,0,0,7951,7932,0,7733,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36481.735144][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895]
[36481.735187][ T3040] [wlan][3040]kalSetPerfReport:(SW4 INFO) Rate[1560][0][0][0] RCPI[102][0][0][0]
[36481.736376][ T3040] [wlan][3040]cnmTimerStartTimer:(CNM INFO) In DoTimeOut, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895] 1000 ms timercount 0
[36481.736454][ T3040] [wlan][3040]wlanLinkQualityMonitor:(SW4 INFO) Link Quality: Tx(rate:7020, total:9574, retry:1616, fail:1483, RTS fail:17, ACK fail:1466), Rx(rate:240, total:8550, dup:0, error:1970), PER(0), Congestion(idle slot:27525222, diff:1351, AwakeDur:282419)
[36481.746349][T200248] mtk_disp_tdshp_stop, line: 528
[36481.746384][T200248] mtk_disp_c3d_stop, line: 909
[36481.746449][T200248] mtk_disp_tdshp_stop, line: 528
[36481.746471][T200248] mtk_disp_c3d_stop, line: 909
[36481.747094][T103042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[10253 us]
[36481.748306][T103042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000811babcd, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=191
[36481.748321][T300248] mtk_disp_tdshp_unprepare
[36481.749516][T103042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000e69040d4, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=192
[36481.749529][T300248] mtk_disp_c3d_unprepare, line: 948
[36481.749548][T25910] [connlog] wifi_fw irq counter = 2797
[36481.749566][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36481.749586][T300248] mtk_dmdp_aal_unprepare
[36481.749597][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36481.749941][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36481.749981][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36481.750077][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36481.750119][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36481.750161][T103042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[34:4:2:0:0] C:[10:20:50:80]=[40:0:0:0:0] M:[5:10:20:50]=[40:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36481.750226][T103042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23270 3216] T[2243 2243 2240 / 10820 10820 10820 10820] R[7951 / 11766] T_R[0 0 0 3 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4150/4150] txreg[7777] rxreg[16594]
[36481.750434][T300248] mtk_disp_tdshp_unprepare
[36481.750602][T300248] mtk_disp_c3d_unprepare, line: 948
[36481.750635][T300248] mtk_dmdp_aal_unprepare
[36481.751343][T203040] [wlan][3040]nicRxProcessEventPacket:(RX INFO) Not static config event: id=0x21, seq=191
[36481.752508][T203040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000811babcd, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=191
[36481.752522][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36481.753733][T203040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000e69040d4, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=192
[36481.753747][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36481.753945][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36481.754025][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36481.754074][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36481.754118][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36481.754160][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36481.776470][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000000
[36482.152021][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7776 us]
[36482.153645][ T3042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 000000001d70cfc0, nicCmdEventQueryLinkStats.cfi_jt [wlan_drv_gen4m_6895], cmd=0x84, seq=193
[36482.158295][T203040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 000000001d70cfc0, nicCmdEventQueryLinkStats.cfi_jt [wlan_drv_gen4m_6895], cmd=0x84, seq=193
[36482.168507][T203042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 0000000069ece6cc, nicCmdEventQueryLinkSpeedEx.cfi_jt [wlan_drv_gen4m_6895], cmd=0x81, seq=194
[36482.173350][T303040] [wlan][3040]nicUpdateLinkQuality:(RLM INFO) Rssi=-51, NewRssi=-51
[36482.173889][T303040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 0000000069ece6cc, nicCmdEventQueryLinkSpeedEx.cfi_jt [wlan_drv_gen4m_6895], cmd=0x81, seq=194
[36482.173946][T303040] [wlan][3040]soc7_0_get_rx_rate_info:(SW4 ERROR) prStaRecOfAP is null
[36482.174848][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000000
[36482.180384][T201462] [wlan][1462]mtk_cfg80211_get_station:(REQ INFO) link speed=8667/8667, bw=0/0, rssi=-51, BSSID:[04:67:**:**:**:10],TxFail=0, TxTimeOut=0, TxOK=10820, RxOK=7951, FcsErr=1
[36482.239571][T301344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.239793][T301344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.239900][T301344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.243323][ T1344] mtk_disp_tdshp_prepare
[36482.244207][ T1344] mtk_dmdp_aal_prepare
[36482.245200][ T1344] mtk_disp_tdshp_prepare
[36482.246067][ T1344] mtk_dmdp_aal_prepare
[36482.247026][ T1344] [cmdq] cmdq_util_enable_disp_va
[36482.247435][ T1344] mtk_disp_tdshp_config, line: 411
[36482.247448][ T1344] mtk_disp_tdshp_start, line: 520
[36482.247456][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.247532][ T1344] mtk_disp_c3d_config, line: 873
[36482.247542][ T1344] mtk_disp_c3d_start, line: 890
[36482.249056][ T1344] [disp_aal]Cannot find LED node from dts
[36482.249065][ T1344] [disp_aal]get pwm cust info fail
[36482.249072][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36482.249947][ T1344] [DISP]mtk_cm_config111
[36482.250016][ T1344] mtk_disp_tdshp_config, line: 411
[36482.250024][ T1344] mtk_disp_tdshp_start, line: 520
[36482.250030][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.250079][ T1344] mtk_disp_c3d_config, line: 873
[36482.250089][ T1344] mtk_disp_c3d_start, line: 890
[36482.250527][ T1344] [disp_aal]Cannot find LED node from dts
[36482.250534][ T1344] [disp_aal]get pwm cust info fail
[36482.250540][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36482.251340][ T1344] [DISP]mtk_cm_config111
[36482.251603][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36482.286577][  T248] mtk_disp_tdshp_stop, line: 528
[36482.286655][  T248] mtk_disp_c3d_stop, line: 909
[36482.286746][  T248] mtk_disp_tdshp_stop, line: 528
[36482.286775][  T248] mtk_disp_c3d_stop, line: 909
[36482.288283][  T248] mtk_disp_tdshp_unprepare
[36482.288496][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.288536][  T248] mtk_dmdp_aal_unprepare
[36482.289391][  T248] mtk_disp_tdshp_unprepare
[36482.289582][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.289624][  T248] mtk_dmdp_aal_unprepare
[36482.291880][T225960] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.292379][T225960] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.292430][T225960] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.292496][T225960] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.292528][T225960] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.292558][T225960] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.292582][T225960] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.292995][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.293405][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.293538][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.299899][  T254] mtk_disp_tdshp_prepare
[36482.300848][  T254] mtk_dmdp_aal_prepare
[36482.301961][  T254] mtk_disp_tdshp_prepare
[36482.303755][  T254] mtk_dmdp_aal_prepare
[36482.304791][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.305248][  T254] mtk_disp_tdshp_config, line: 411
[36482.305261][  T254] mtk_disp_tdshp_start, line: 520
[36482.305270][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.305347][  T254] mtk_disp_c3d_config, line: 873
[36482.305357][  T254] mtk_disp_c3d_start, line: 890
[36482.306910][  T254] [disp_aal]Cannot find LED node from dts
[36482.306922][  T254] [disp_aal]get pwm cust info fail
[36482.306929][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.307761][  T254] [DISP]mtk_cm_config111
[36482.307821][  T254] mtk_disp_tdshp_config, line: 411
[36482.307828][  T254] mtk_disp_tdshp_start, line: 520
[36482.307834][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.307874][  T254] mtk_disp_c3d_config, line: 873
[36482.307884][  T254] mtk_disp_c3d_start, line: 890
[36482.308236][  T254] [disp_aal]Cannot find LED node from dts
[36482.308244][  T254] [disp_aal]get pwm cust info fail
[36482.308250][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.309046][  T254] [DISP]mtk_cm_config111
[36482.309292][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36482.346996][  T248] mtk_disp_tdshp_stop, line: 528
[36482.347061][  T248] mtk_disp_c3d_stop, line: 909
[36482.347142][  T248] mtk_disp_tdshp_stop, line: 528
[36482.347170][  T248] mtk_disp_c3d_stop, line: 909
[36482.348906][  T248] mtk_disp_tdshp_unprepare
[36482.349133][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.349214][  T248] mtk_dmdp_aal_unprepare
[36482.350187][T100248] mtk_disp_tdshp_unprepare
[36482.350404][T100248] mtk_disp_c3d_unprepare, line: 948
[36482.350448][T100248] mtk_dmdp_aal_unprepare
[36482.352649][T225960] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.353058][T225960] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.353109][T225960] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.353174][T225960] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.353205][T225960] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.353234][T225960] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.353259][T225960] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.353672][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.354198][T200254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.354329][T200254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.357209][  T254] mtk_disp_tdshp_prepare
[36482.358031][  T254] mtk_dmdp_aal_prepare
[36482.358946][  T254] mtk_disp_tdshp_prepare
[36482.359727][  T254] mtk_dmdp_aal_prepare
[36482.360610][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.360949][  T254] mtk_disp_tdshp_config, line: 411
[36482.360960][  T254] mtk_disp_tdshp_start, line: 520
[36482.360969][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.361042][  T254] mtk_disp_c3d_config, line: 873
[36482.361052][  T254] mtk_disp_c3d_start, line: 890
[36482.362522][  T254] [disp_aal]Cannot find LED node from dts
[36482.362532][  T254] [disp_aal]get pwm cust info fail
[36482.362539][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.363338][  T254] [DISP]mtk_cm_config111
[36482.363388][  T254] mtk_disp_tdshp_config, line: 411
[36482.363395][  T254] mtk_disp_tdshp_start, line: 520
[36482.363401][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.363436][  T254] mtk_disp_c3d_config, line: 873
[36482.363446][  T254] mtk_disp_c3d_start, line: 890
[36482.363776][  T254] [disp_aal]Cannot find LED node from dts
[36482.363783][  T254] [disp_aal]get pwm cust info fail
[36482.363789][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.364553][  T254] [DISP]mtk_cm_config111
[36482.364795][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36482.381923][T125341] sd 0:0:0:2: [sdc] Synchronizing SCSI cache
[36482.398371][  T248] mtk_disp_tdshp_stop, line: 528
[36482.398410][  T248] mtk_disp_c3d_stop, line: 909
[36482.398486][  T248] mtk_disp_tdshp_stop, line: 528
[36482.398509][  T248] mtk_disp_c3d_stop, line: 909
[36482.399891][  T248] mtk_disp_tdshp_unprepare
[36482.400057][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.400091][  T248] mtk_dmdp_aal_unprepare
[36482.400809][  T248] mtk_disp_tdshp_unprepare
[36482.400973][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.401006][  T248] mtk_dmdp_aal_unprepare
[36482.402528][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.402715][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.402756][T25910] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.402813][T25910] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.402838][T25910] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.402861][T25910] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.402880][T25910] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.403753][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.403932][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.404039][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.407790][  T254] mtk_disp_tdshp_prepare
[36482.408738][  T254] mtk_dmdp_aal_prepare
[36482.409820][  T254] mtk_disp_tdshp_prepare
[36482.410613][  T254] mtk_dmdp_aal_prepare
[36482.411535][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.411918][  T254] mtk_disp_tdshp_config, line: 411
[36482.411929][  T254] mtk_disp_tdshp_start, line: 520
[36482.411937][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.411993][  T254] mtk_disp_c3d_config, line: 873
[36482.412004][  T254] mtk_disp_c3d_start, line: 890
[36482.413234][  T254] [disp_aal]Cannot find LED node from dts
[36482.413245][  T254] [disp_aal]get pwm cust info fail
[36482.413251][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.414112][  T254] [DISP]mtk_cm_config111
[36482.414168][  T254] mtk_disp_tdshp_config, line: 411
[36482.414175][  T254] mtk_disp_tdshp_start, line: 520
[36482.414181][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.414215][  T254] mtk_disp_c3d_config, line: 873
[36482.414225][  T254] mtk_disp_c3d_start, line: 890
[36482.414550][  T254] [disp_aal]Cannot find LED node from dts
[36482.414557][  T254] [disp_aal]get pwm cust info fail
[36482.414562][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.415323][  T254] [DISP]mtk_cm_config111
[36482.415562][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36482.452029][  T248] mtk_disp_tdshp_stop, line: 528
[36482.452107][  T248] mtk_disp_c3d_stop, line: 909
[36482.452192][  T248] mtk_disp_tdshp_stop, line: 528
[36482.452219][  T248] mtk_disp_c3d_stop, line: 909
[36482.453678][  T248] mtk_disp_tdshp_unprepare
[36482.454086][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.454126][  T248] mtk_dmdp_aal_unprepare
[36482.455090][  T248] mtk_disp_tdshp_unprepare
[36482.455280][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.455321][  T248] mtk_dmdp_aal_unprepare
[36482.457111][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.457355][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.457402][T125341] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.457464][T125341] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.457495][T125341] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.457524][T125341] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.457548][T125341] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.458478][T200254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.458970][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.459102][  T254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.462190][T100254] mtk_disp_tdshp_prepare
[36482.463030][  T254] mtk_dmdp_aal_prepare
[36482.464108][  T254] mtk_disp_tdshp_prepare
[36482.464910][  T254] mtk_dmdp_aal_prepare
[36482.466034][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.466398][  T254] mtk_disp_tdshp_config, line: 411
[36482.466412][  T254] mtk_disp_tdshp_start, line: 520
[36482.466420][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.466481][  T254] mtk_disp_c3d_config, line: 873
[36482.466492][  T254] mtk_disp_c3d_start, line: 890
[36482.467830][  T254] [disp_aal]Cannot find LED node from dts
[36482.467841][  T254] [disp_aal]get pwm cust info fail
[36482.467848][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.468773][  T254] [DISP]mtk_cm_config111
[36482.468817][  T254] mtk_disp_tdshp_config, line: 411
[36482.468824][  T254] mtk_disp_tdshp_start, line: 520
[36482.468830][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.468862][  T254] mtk_disp_c3d_config, line: 873
[36482.468871][  T254] mtk_disp_c3d_start, line: 890
[36482.469224][  T254] [disp_aal]Cannot find LED node from dts
[36482.469231][  T254] [disp_aal]get pwm cust info fail
[36482.469237][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.470057][  T254] [DISP]mtk_cm_config111
[36482.470285][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36482.507046][  T248] mtk_disp_tdshp_stop, line: 528
[36482.507126][  T248] mtk_disp_c3d_stop, line: 909
[36482.507214][  T248] mtk_disp_tdshp_stop, line: 528
[36482.507243][  T248] mtk_disp_c3d_stop, line: 909
[36482.508886][  T248] mtk_disp_tdshp_unprepare
[36482.509192][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.509235][  T248] mtk_dmdp_aal_unprepare
[36482.510438][T100248] mtk_disp_tdshp_unprepare
[36482.510641][T100248] mtk_disp_c3d_unprepare, line: 948
[36482.510683][T100248] mtk_dmdp_aal_unprepare
[36482.512534][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.512756][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.512805][T25910] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.512870][T25910] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.512902][T25910] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.512931][T25910] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.512955][T25910] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.513677][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.514045][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.514182][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.517326][  T254] mtk_disp_tdshp_prepare
[36482.519420][  T254] mtk_dmdp_aal_prepare
[36482.520674][  T254] mtk_disp_tdshp_prepare
[36482.521515][  T254] mtk_dmdp_aal_prepare
[36482.522696][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.523159][  T254] mtk_disp_tdshp_config, line: 411
[36482.523173][  T254] mtk_disp_tdshp_start, line: 520
[36482.523182][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.523264][  T254] mtk_disp_c3d_config, line: 873
[36482.523275][  T254] mtk_disp_c3d_start, line: 890
[36482.524953][  T254] [disp_aal]Cannot find LED node from dts
[36482.524966][  T254] [disp_aal]get pwm cust info fail
[36482.524973][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.525968][  T254] [DISP]mtk_cm_config111
[36482.526031][  T254] mtk_disp_tdshp_config, line: 411
[36482.526039][  T254] mtk_disp_tdshp_start, line: 520
[36482.526045][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.526082][  T254] mtk_disp_c3d_config, line: 873
[36482.526093][  T254] mtk_disp_c3d_start, line: 890
[36482.526559][  T254] [disp_aal]Cannot find LED node from dts
[36482.526568][  T254] [disp_aal]get pwm cust info fail
[36482.526576][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.527480][  T254] [DISP]mtk_cm_config111
[36482.527764][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36482.562606][  T248] mtk_disp_tdshp_stop, line: 528
[36482.562684][  T248] mtk_disp_c3d_stop, line: 909
[36482.562774][  T248] mtk_disp_tdshp_stop, line: 528
[36482.562803][  T248] mtk_disp_c3d_stop, line: 909
[36482.564297][  T248] mtk_disp_tdshp_unprepare
[36482.564513][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.564554][  T248] mtk_dmdp_aal_unprepare
[36482.565429][  T248] mtk_disp_tdshp_unprepare
[36482.565642][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.565901][  T248] mtk_dmdp_aal_unprepare
[36482.567956][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.568377][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.568428][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.568493][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.568524][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.568553][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.568577][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.568991][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.569239][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.569369][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.572468][  T254] mtk_disp_tdshp_prepare
[36482.573315][  T254] mtk_dmdp_aal_prepare
[36482.574478][  T254] mtk_disp_tdshp_prepare
[36482.575289][  T254] mtk_dmdp_aal_prepare
[36482.576332][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.576769][  T254] mtk_disp_tdshp_config, line: 411
[36482.576784][  T254] mtk_disp_tdshp_start, line: 520
[36482.576793][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.576871][  T254] mtk_disp_c3d_config, line: 873
[36482.576881][  T254] mtk_disp_c3d_start, line: 890
[36482.578508][  T254] [disp_aal]Cannot find LED node from dts
[36482.578522][  T254] [disp_aal]get pwm cust info fail
[36482.578529][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.579469][  T254] [DISP]mtk_cm_config111
[36482.579526][  T254] mtk_disp_tdshp_config, line: 411
[36482.579533][  T254] mtk_disp_tdshp_start, line: 520
[36482.579540][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.579577][  T254] mtk_disp_c3d_config, line: 873
[36482.579587][  T254] mtk_disp_c3d_start, line: 890
[36482.580051][  T254] [disp_aal]Cannot find LED node from dts
[36482.580061][  T254] [disp_aal]get pwm cust info fail
[36482.580068][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.581014][  T254] [DISP]mtk_cm_config111
[36482.581300][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36482.690193][  T248] mtk_disp_tdshp_stop, line: 528
[36482.690273][  T248] mtk_disp_c3d_stop, line: 909
[36482.690393][  T248] mtk_disp_tdshp_stop, line: 528
[36482.690423][  T248] mtk_disp_c3d_stop, line: 909
[36482.691919][  T248] mtk_disp_tdshp_unprepare
[36482.692135][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.692176][  T248] mtk_dmdp_aal_unprepare
[36482.693065][  T248] mtk_disp_tdshp_unprepare
[36482.693254][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.693296][  T248] mtk_dmdp_aal_unprepare
[36482.695138][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.695362][T25910] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.695409][T25910] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.695473][T25910] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.695503][T25910] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.695530][T25910] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.695554][T25910] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.734604][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5031
[36482.736250][T726018] i2c_error_count_get 0
[36482.736280][T726018] authentic_get 1
[36482.736306][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36482.736321][T726018] charge_done_get 1
[36482.736339][T726018] capacity_raw_get 9904
[36482.736354][T726018] fastcharge_mode_set 0
[36482.736366][T726018] monitor_delay_set 30000
[36482.736384][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36482.736397][T726018] capacity_raw_get 9904
[36482.736411][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36482.736422][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36482.736430][T726018] handle_step_charge index = 3
[36482.736435][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36482.736446][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36482.736455][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36482.742837][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36482.742869][T726018] connector_temp_get 243
[36482.742880][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36482.757946][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1024ms> Tput: 0(0.000mbps) [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:47805] lv:0 th:5 fg:0x9 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:291
[36482.758010][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13170,0,0,7951,7932,0,7733,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36482.759196][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895]
[36482.760438][ T3040] [wlan][3040]cnmTimerStartTimer:(CNM INFO) In DoTimeOut, timer 0000000030f6d473 func kalPerMonHandler.cfi_jt [wlan_drv_gen4m_6895] 1000 ms timercount 0
[36482.760483][ T3040] asicConnac2xFillCmdTxdInfo: 6 callbacks suppressed
[36482.760501][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x85] SEQ[195] SET[0] LEN[92]
[36482.760543][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x82] SEQ[196] SET[0] LEN[248]
[36482.760581][ T3040] [wlan][3040]wlanLinkQualityMonitor:(SW4 INFO) Link Quality: Tx(rate:7020, total:9615, retry:1619, fail:1486, RTS fail:17, ACK fail:1469), Rx(rate:240, total:8591, dup:0, error:1971), PER(7), Congestion(idle slot:27573027, diff:47805, AwakeDur:282895)
[36482.768832][T103042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7979 us]
[36482.770037][T103042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000dbab8053, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=195
[36482.770050][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36482.770090][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36482.771305][T103042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000e75a1378, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=196
[36482.771564][T103042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:0:0:0] C:[10:20:50:80]=[0:0:0:0:0] M:[5:10:20:50]=[0:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36482.771632][T103042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23278 3217] T[2247 2247 2245 / 10820 10820 10820 10820] R[7951 / 11772] T_R[0 0 0 2 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4150/4150] txreg[7777] rxreg[16600]
[36482.773243][T303040] [wlan][3040]nicRxProcessEventPacket:(RX INFO) Not static config event: id=0x21, seq=195
[36482.774402][T303040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000dbab8053, nicCmdEventQueryStaStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x85, seq=195
[36482.775655][T303040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000e75a1378, nicCmdEventQueryStatistics.cfi_jt [wlan_drv_gen4m_6895], cmd=0x82, seq=196
[36482.795019][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000000
[36482.813144][T201344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.813377][T301344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.813450][T301344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.816361][ T1344] mtk_disp_tdshp_prepare
[36482.817204][ T1344] mtk_dmdp_aal_prepare
[36482.818103][ T1344] mtk_disp_tdshp_prepare
[36482.818885][ T1344] mtk_dmdp_aal_prepare
[36482.819709][ T1344] [cmdq] cmdq_util_enable_disp_va
[36482.820060][ T1344] mtk_disp_tdshp_config, line: 411
[36482.820070][ T1344] mtk_disp_tdshp_start, line: 520
[36482.820078][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.820152][ T1344] mtk_disp_c3d_config, line: 873
[36482.820161][ T1344] mtk_disp_c3d_start, line: 890
[36482.821518][ T1344] [disp_aal]Cannot find LED node from dts
[36482.821526][ T1344] [disp_aal]get pwm cust info fail
[36482.821532][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36482.822146][C400000] [name:spm&][RC] ratio, duration_ms:10240, bus26m:15%, syspll:0%, dram:3%
[36482.822349][T423900] [ccci1/net]to:clr(0:0)

[36482.822444][T423900] [ccci1/net]ccmni0(1,1), irat_MD1, rx=(4,324,488), tx=(1,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x3,0x81,0x0,0x0)
[36482.822475][T423900] [ccci1/net]to:clr(0:0)

[36482.822534][T423900] [ccci1/net]ccmni1(0,0), irat_MD1, rx=(1028,677154,488), tx=(926,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x2,0x80,0x1,0x1)
[36482.822562][T423900] [ccci1/net]to:clr(0:0)

[36482.822618][T423900] [ccci1/net]ccmni2(0,0), irat_MD1, rx=(24,6251,488), tx=(29,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x2,0x80,0x1,0x1)
[36482.822648][ T1344] [DISP]mtk_cm_config111
[36482.822713][ T1344] mtk_disp_tdshp_config, line: 411
[36482.822719][ T1344] mtk_disp_tdshp_start, line: 520
[36482.822725][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.822761][ T1344] mtk_disp_c3d_config, line: 873
[36482.822770][ T1344] mtk_disp_c3d_start, line: 890
[36482.823141][T423900] [ccci1/cif]total cnt=14253;rxq0 isr_cnt=2420;rxq1 isr_cnt=81;rxq2 isr_cnt=0;rxq3 isr_cnt=2;rxq4 isr_cnt=3468;rxq5 isr_cnt=8267;rxq6 isr_cnt=1;rxq7 isr_cnt=18;rxq8 isr_cnt=0;rxq9 isr_cnt=0;rxq10 isr_cnt=0;rxq11 isr_cnt=0;rxq12 isr_cnt=0;rxq13 isr_cnt=0;rxq14 isr_cnt=0;rxq15 isr_cnt=1;rxq16 isr_cnt=0;rxq17 isr_cnt=0;rxq18 isr_cnt=0;rxq19 isr_cnt=0;rxq20 isr_cnt=0;rxq21 isr_cnt=0;rxq22 isr_cnt=0;rxq23 isr_cnt=0;
[36482.823183][ T1344] [disp_aal]Cannot find LED node from dts
[36482.823190][ T1344] [disp_aal]get pwm cust info fail
[36482.823196][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36482.823886][ T1344] [DISP]mtk_cm_config111
[36482.824113][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36482.829833][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7538 us]
[36482.862564][  T248] mtk_disp_tdshp_stop, line: 528
[36482.862643][  T248] mtk_disp_c3d_stop, line: 909
[36482.862733][  T248] mtk_disp_tdshp_stop, line: 528
[36482.862762][  T248] mtk_disp_c3d_stop, line: 909
[36482.864263][  T248] mtk_disp_tdshp_unprepare
[36482.864476][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.864517][  T248] mtk_dmdp_aal_unprepare
[36482.864871][  T248] mtk_disp_tdshp_unprepare
[36482.865042][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.865080][  T248] mtk_dmdp_aal_unprepare
[36482.869130][  T254] mtk_disp_tdshp_prepare
[36482.870951][  T254] mtk_dmdp_aal_prepare
[36482.871942][  T254] mtk_disp_tdshp_prepare
[36482.872737][  T254] mtk_dmdp_aal_prepare
[36482.873598][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.874165][  T254] mtk_disp_tdshp_config, line: 411
[36482.874176][  T254] mtk_disp_tdshp_start, line: 520
[36482.874184][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.874259][  T254] mtk_disp_c3d_config, line: 873
[36482.874269][  T254] mtk_disp_c3d_start, line: 890
[36482.875640][  T254] [disp_aal]Cannot find LED node from dts
[36482.875647][  T254] [disp_aal]get pwm cust info fail
[36482.875654][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.876362][  T254] [DISP]mtk_cm_config111
[36482.876416][  T254] mtk_disp_tdshp_config, line: 411
[36482.876423][  T254] mtk_disp_tdshp_start, line: 520
[36482.876429][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.876462][  T254] mtk_disp_c3d_config, line: 873
[36482.876471][  T254] mtk_disp_c3d_start, line: 890
[36482.876757][  T254] [disp_aal]Cannot find LED node from dts
[36482.876764][  T254] [disp_aal]get pwm cust info fail
[36482.876769][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.877462][  T254] [DISP]mtk_cm_config111
[36482.877729][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36482.896232][T103042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36482.914661][T100248] mtk_disp_tdshp_stop, line: 528
[36482.914741][T100248] mtk_disp_c3d_stop, line: 909
[36482.914830][T100248] mtk_disp_tdshp_stop, line: 528
[36482.914859][T100248] mtk_disp_c3d_stop, line: 909
[36482.916467][T100248] mtk_disp_tdshp_unprepare
[36482.916792][T100248] mtk_disp_c3d_unprepare, line: 948
[36482.916837][T100248] mtk_dmdp_aal_unprepare
[36482.917679][T100248] mtk_disp_tdshp_unprepare
[36482.918048][T100248] mtk_disp_c3d_unprepare, line: 948
[36482.918113][T100248] mtk_dmdp_aal_unprepare
[36482.920396][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.920642][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.920691][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.920756][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.920787][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.920815][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.920840][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.921247][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.921654][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.921881][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.925619][  T254] mtk_disp_tdshp_prepare
[36482.926768][  T254] mtk_dmdp_aal_prepare
[36482.927643][  T254] mtk_disp_tdshp_prepare
[36482.928421][  T254] mtk_dmdp_aal_prepare
[36482.929255][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.929616][  T254] mtk_disp_tdshp_config, line: 411
[36482.929626][  T254] mtk_disp_tdshp_start, line: 520
[36482.929634][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.929743][  T254] mtk_disp_c3d_config, line: 873
[36482.929754][  T254] mtk_disp_c3d_start, line: 890
[36482.931151][  T254] [disp_aal]Cannot find LED node from dts
[36482.931158][  T254] [disp_aal]get pwm cust info fail
[36482.931165][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.931874][  T254] [DISP]mtk_cm_config111
[36482.931916][  T254] mtk_disp_tdshp_config, line: 411
[36482.931923][  T254] mtk_disp_tdshp_start, line: 520
[36482.931929][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.931957][  T254] mtk_disp_c3d_config, line: 873
[36482.931999][  T254] mtk_disp_c3d_start, line: 890
[36482.932289][  T254] [disp_aal]Cannot find LED node from dts
[36482.932296][  T254] [disp_aal]get pwm cust info fail
[36482.932301][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.932983][  T254] [DISP]mtk_cm_config111
[36482.933203][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36482.971053][  T248] mtk_disp_tdshp_stop, line: 528
[36482.971133][  T248] mtk_disp_c3d_stop, line: 909
[36482.971222][  T248] mtk_disp_tdshp_stop, line: 528
[36482.971251][  T248] mtk_disp_c3d_stop, line: 909
[36482.972742][  T248] mtk_disp_tdshp_unprepare
[36482.972964][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.973005][  T248] mtk_dmdp_aal_unprepare
[36482.974185][  T248] mtk_disp_tdshp_unprepare
[36482.974397][  T248] mtk_disp_c3d_unprepare, line: 948
[36482.974439][  T248] mtk_dmdp_aal_unprepare
[36482.976500][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36482.976750][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36482.976800][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36482.976865][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36482.976895][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.976924][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36482.976948][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36482.977353][  T254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36482.977886][T100254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36482.978030][T100254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36482.980989][  T254] mtk_disp_tdshp_prepare
[36482.982861][  T254] mtk_dmdp_aal_prepare
[36482.983831][  T254] mtk_disp_tdshp_prepare
[36482.984626][  T254] mtk_dmdp_aal_prepare
[36482.985455][  T254] [cmdq] cmdq_util_enable_disp_va
[36482.985924][  T254] mtk_disp_tdshp_config, line: 411
[36482.985935][  T254] mtk_disp_tdshp_start, line: 520
[36482.985943][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.986017][  T254] mtk_disp_c3d_config, line: 873
[36482.986026][  T254] mtk_disp_c3d_start, line: 890
[36482.987398][  T254] [disp_aal]Cannot find LED node from dts
[36482.987407][  T254] [disp_aal]get pwm cust info fail
[36482.987413][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.988119][  T254] [DISP]mtk_cm_config111
[36482.988169][  T254] mtk_disp_tdshp_config, line: 411
[36482.988176][  T254] mtk_disp_tdshp_start, line: 520
[36482.988181][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36482.988214][  T254] mtk_disp_c3d_config, line: 873
[36482.988223][  T254] mtk_disp_c3d_start, line: 890
[36482.988512][  T254] [disp_aal]Cannot find LED node from dts
[36482.988518][  T254] [disp_aal]get pwm cust info fail
[36482.988524][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36482.989204][  T254] [DISP]mtk_cm_config111
[36482.989418][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36483.026960][  T248] mtk_disp_tdshp_stop, line: 528
[36483.027034][  T248] mtk_disp_c3d_stop, line: 909
[36483.027120][  T248] mtk_disp_tdshp_stop, line: 528
[36483.027150][  T248] mtk_disp_c3d_stop, line: 909
[36483.028624][  T248] mtk_disp_tdshp_unprepare
[36483.028838][  T248] mtk_disp_c3d_unprepare, line: 948
[36483.028879][  T248] mtk_dmdp_aal_unprepare
[36483.029922][  T248] mtk_disp_tdshp_unprepare
[36483.030108][  T248] mtk_disp_c3d_unprepare, line: 948
[36483.030157][  T248] mtk_dmdp_aal_unprepare
[36483.032271][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36483.032528][T125341] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36483.032576][T125341] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36483.032633][T125341] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36483.032663][T125341] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36483.032691][T125341] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36483.032715][T125341] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36483.033108][T200254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36483.033336][T200254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36483.033457][T200254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36483.036727][  T254] mtk_disp_tdshp_prepare
[36483.037527][  T254] mtk_dmdp_aal_prepare
[36483.038347][  T254] mtk_disp_tdshp_prepare
[36483.040349][  T254] mtk_dmdp_aal_prepare
[36483.041306][  T254] [cmdq] cmdq_util_enable_disp_va
[36483.041829][  T254] mtk_disp_tdshp_config, line: 411
[36483.041841][  T254] mtk_disp_tdshp_start, line: 520
[36483.041849][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36483.041924][  T254] mtk_disp_c3d_config, line: 873
[36483.041934][  T254] mtk_disp_c3d_start, line: 890
[36483.043253][  T254] [disp_aal]Cannot find LED node from dts
[36483.043260][  T254] [disp_aal]get pwm cust info fail
[36483.043267][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36483.043963][  T254] [DISP]mtk_cm_config111
[36483.044003][  T254] mtk_disp_tdshp_config, line: 411
[36483.044009][  T254] mtk_disp_tdshp_start, line: 520
[36483.044014][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36483.044045][  T254] mtk_disp_c3d_config, line: 873
[36483.044053][  T254] mtk_disp_c3d_start, line: 890
[36483.044340][  T254] [disp_aal]Cannot find LED node from dts
[36483.044345][  T254] [disp_aal]get pwm cust info fail
[36483.044351][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36483.045028][  T254] [DISP]mtk_cm_config111
[36483.045232][  T254] [DISP]mtk_dsc_config+ pad_num:0
[36483.078141][C600000] [ccci1/dpmaif]net txq0-3(status=0xf)[2048]:2048-404-404(0x0), 2048-551-551(0x0), 2048-0-0(0x0), 2048-1-1(0x0)
[36483.078328][C600000] [ccci1/dpmaif]Current txq pos: w/r/rel=(808,808,808)(1102,1102,1102)(0,0,0)(2,2,2), tx_busy=0,0,0,0
[36483.118889][T100248] mtk_disp_tdshp_stop, line: 528
[36483.118959][T100248] mtk_disp_c3d_stop, line: 909
[36483.119044][T100248] mtk_disp_tdshp_stop, line: 528
[36483.119072][T100248] mtk_disp_c3d_stop, line: 909
[36483.120838][  T248] mtk_disp_tdshp_unprepare
[36483.121060][  T248] mtk_disp_c3d_unprepare, line: 948
[36483.121099][  T248] mtk_dmdp_aal_unprepare
[36483.122135][  T248] mtk_disp_tdshp_unprepare
[36483.122334][  T248] mtk_disp_c3d_unprepare, line: 948
[36483.122375][  T248] mtk_dmdp_aal_unprepare
[36483.124356][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36483.124599][T325406] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36483.124645][T325406] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36483.124711][T325406] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36483.124741][T325406] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36483.124770][T325406] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36483.124794][T325406] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36483.337900][C523641] [name:spm&][SPM] system_bus wake up by  R12_SYSTIMER, timer_out = 212, r13 = 0x80001bc4, debug_flag = 0x10000000 0x3, r12 = 0x40000, r12_ext = 0x0, raw_sta = 0x0 0x0 0x0, idle_sta = 0x0, req_sta =  0x0 0x1f000000 0x1f0 0x3e3e001a 0xa 0x3fd000 0x0, cg_check_sta =0x1f0, isr = 0x0, rt_req_sta0 = 0x0 rt_req_sta1 = 0x0 rt_req_sta2 = 0xffffffff rt_req_sta3 = 0xffffffff dram_sw_con_3 = 0x0, raw_ext_sta = 0x208a55, wake_misc = 0x180030, pcm_flag = 0x2890077 0x0 0x2810077 0x2810077, req = 0x6360200,  clk_settle = 0x60fe, 
[36483.388328][T201344] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36483.388576][T101344] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36483.388662][T101344] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36483.391763][T101344] mtk_disp_tdshp_prepare
[36483.392679][ T1344] mtk_dmdp_aal_prepare
[36483.393862][ T1344] mtk_disp_tdshp_prepare
[36483.394686][ T1344] mtk_dmdp_aal_prepare
[36483.395775][ T1344] [cmdq] cmdq_util_enable_disp_va
[36483.396202][ T1344] mtk_disp_tdshp_config, line: 411
[36483.396216][ T1344] mtk_disp_tdshp_start, line: 520
[36483.396225][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36483.396306][ T1344] mtk_disp_c3d_config, line: 873
[36483.396317][ T1344] mtk_disp_c3d_start, line: 890
[36483.398000][ T1344] [disp_aal]Cannot find LED node from dts
[36483.398014][ T1344] [disp_aal]get pwm cust info fail
[36483.398022][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36483.398957][ T1344] [DISP]mtk_cm_config111
[36483.399013][ T1344] mtk_disp_tdshp_config, line: 411
[36483.399021][ T1344] mtk_disp_tdshp_start, line: 520
[36483.399028][ T1344] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36483.399067][ T1344] mtk_disp_c3d_config, line: 873
[36483.399077][ T1344] mtk_disp_c3d_start, line: 890
[36483.399635][ T1344] [disp_aal]Cannot find LED node from dts
[36483.399645][ T1344] [disp_aal]get pwm cust info fail
[36483.399651][ T1344] [disp_aal]disp_aal_get_cust_led mode=0
[36483.400447][ T1344] [DISP]mtk_cm_config111
[36483.400696][ T1344] [DISP]mtk_dsc_config+ pad_num:0
[36483.433919][T400248] mtk_disp_tdshp_stop, line: 528
[36483.433934][T400248] mtk_disp_c3d_stop, line: 909
[36483.433950][T400248] mtk_disp_tdshp_stop, line: 528
[36483.433953][T400248] mtk_disp_c3d_stop, line: 909
[36483.434836][T400248] mtk_disp_tdshp_unprepare
[36483.435009][T400248] mtk_disp_c3d_unprepare, line: 948
[36483.435047][T400248] mtk_dmdp_aal_unprepare
[36483.435309][T400248] mtk_disp_tdshp_unprepare
[36483.435371][T400248] mtk_disp_c3d_unprepare, line: 948
[36483.435394][T400248] mtk_dmdp_aal_unprepare
[36483.437007][T423900] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36483.437178][T423900] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36483.437207][T423900] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36483.437242][T423900] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36483.437258][T423900] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36483.437270][T423900] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36483.437274][T423900] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36483.440399][T600254] do_mminfra_bkrs: call scmi_tinysys_common_set(1)
[36483.440720][T700254] do_mminfra_bkrs: call scmi_tinysys_common_set(1) err=0
[36483.440794][T700254] mtk_mminfra_pd_callback: enable clk ref_cnt=1
[36483.444050][T400254] mtk_disp_tdshp_prepare
[36483.444843][T400254] mtk_dmdp_aal_prepare
[36483.445647][T400254] mtk_disp_tdshp_prepare
[36483.446495][T200254] mtk_dmdp_aal_prepare
[36483.447320][T200254] [cmdq] cmdq_util_enable_disp_va
[36483.447725][  T254] mtk_disp_tdshp_config, line: 411
[36483.447736][  T254] mtk_disp_tdshp_start, line: 520
[36483.447742][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36483.447788][  T254] mtk_disp_c3d_config, line: 873
[36483.447797][  T254] mtk_disp_c3d_start, line: 890
[36483.448693][  T254] [disp_aal]Cannot find LED node from dts
[36483.448699][  T254] [disp_aal]get pwm cust info fail
[36483.448705][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36483.449378][  T254] [DISP]mtk_cm_config111
[36483.449429][  T254] mtk_disp_tdshp_config, line: 411
[36483.449436][  T254] mtk_disp_tdshp_start, line: 520
[36483.449442][  T254] tdshp_en: 1, tdshp_limit: 20, tdshp_ylev_256: 56
[36483.449486][  T254] mtk_disp_c3d_config, line: 873
[36483.449495][  T254] mtk_disp_c3d_start, line: 890
[36483.450315][  T254] [disp_aal]Cannot find LED node from dts
[36483.450324][  T254] [disp_aal]get pwm cust info fail
[36483.450330][  T254] [disp_aal]disp_aal_get_cust_led mode=0
[36483.451022][  T254] [DISP]mtk_cm_config111
[36483.451312][T200254] [DISP]mtk_dsc_config+ pad_num:0
[36483.577127][T618352] leds_mtk led_debug_log(165) :[Light] Set lcd-backlight directlyT:36465.528,L:243 L:243 map:61    T:36465.536,L:147 L:147 map:37    T:36465.544,L:51 L:51 map:13    T:36465.552,L:40 L:40 map:10    T:36483.577,L:0 L:0 map:0    
[36483.577353][T618352] lcm_setbacklight_cmdq 0 0 0
[36483.606677][T201180] [DISP]mtk_drm_crtc_mode_check++ from 2 to 2
[36483.606725][T201180] [DISP]lyeblob lost ID:0
[36483.606732][T201180] [DISP]mtk_crtc_update_ddp_state frame:0 correct invalid hrt to:4, mode:2->2
[36483.607054][T201180] [btmtk_info] btmtk_disp_notify_cb: value[0], data[1]
[36483.607065][T201180] [btmtk_info] btmtk_disp_notify_cb: blank state [1]->[0], and send cmd
[36483.607072][T201180] [btmtk_info] [InternalCmd] btmtk_intcmd_wmt_blank_status
[36483.609989][T201180] [btmtk_info] btmtk_btif_send_and_recv, ret = 499
[36483.610010][T201180] [btmtk_info] [InternalCmd] btmtk_intcmd_wmt_blank_status done, result = WMT_EVT_SUCCESS
[36483.610021][T201180] [wlan][1180]wlan_fb_notifier_callback:(SW4 INFO) wlan_fb_notifier_callback: event[0], blank[1]
[36483.610027][T201180] [wlan][1180]kalPerMonDisable:(SW4 INFO) enter kalPerMonDisable
[36483.610037][T201180] [wlan][1180]kalPerMonStop:(SW4 INFO) perf monitor stopped
[36483.610042][T201180] [wlan][1180]wlan_fb_notifier_callback:(SW4 INFO) wlan_fb_notifier_callback: end
[36483.610050][T201180] mi_disp_notifier_call_chain, notify = 2
[36483.610060][T201180] [ FTS ] fts_drm_state_chg_callback: val:2,blank:5
[36483.610067][T201180] screen_state_for_thermal_callback IN val:2,balnk:5
[36483.610072][T201180] screen_state_for_thermal_callback OUT screen_state 1
[36483.610116][T201180] panel esd irq is disable
[36483.610444][T201300] set battery thermal level = 0
[36483.613467][ T1066] [btmtk_info] BT_unlocked_ioctl: cmd[0x4008b004]
[36483.613487][ T1066] [btmtk_info] BT_unlocked_ioctl: id[1], value[0x00000000], desc[BLE_SCAN]
[36483.613540][T201180] lcm_unprepare
[36483.618171][T618388] [btmtk_info] BT_unlocked_ioctl: cmd[0x4008b004]
[36483.618185][T618388] [btmtk_info] BT_unlocked_ioctl: id[1], value[0x12c00140], desc[BLE_SCAN]
[36483.649818][T25115] MTK-BTIF-EXP[I]mtk_wcn_btif_dpidle_ctrl:enter deep idle
[36483.758075][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5043
[36483.759902][T726018] i2c_error_count_get 0
[36483.759930][T726018] authentic_get 1
[36483.759953][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36483.759967][T726018] charge_done_get 1
[36483.759985][T726018] capacity_raw_get 9904
[36483.759999][T726018] fastcharge_mode_set 0
[36483.760011][T726018] monitor_delay_set 30000
[36483.760027][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36483.760037][T726018] capacity_raw_get 9904
[36483.760048][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36483.760058][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36483.760065][T726018] handle_step_charge index = 3
[36483.760070][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36483.760081][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36483.760090][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36483.766487][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36483.766520][T726018] connector_temp_get 243
[36483.766531][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36483.836918][T201180] lcm_panel_vci_disable +
[36483.836977][T201180] lcm_panel_vci_disable regulator_is_enabled = 1
[36483.837016][T201180] lcm_panel_vci_disable -
[36483.839554][T201180] [mi_disp:mtk_output_dsi_disable [mediatek_drm]] [info]power state:5
[36483.839653][T201180] mi_disp_notifier_call_chain, notify = 1
[36483.839664][T201180] [info] fpc_fb_notif_callback value = 1 blank = 5
[36483.839669][T201180] [info] fpc_fb_notif_callback lcd off notify
[36483.839678][T201180] [ FTS ] fts_drm_state_chg_callback: val:1,blank:5
[36483.839683][T201180] [ FTS ] fts_drm_state_chg_callback: FB_BLANK_POWERDOWN
[36483.839720][T201180] screen_state_for_thermal_callback IN val:1,balnk:5
[36483.839726][T201180] screen_state_for_thermal_callback OUT screen_state 0
[36483.839748][T201180] cm_mgr_fb_notifier_callback+
[36483.839756][T201180] #@# cm_mgr_fb_notifier_callback(308) SCREEN OFF
[36483.839764][T314508] [ FTS ] Interrupt Disabled!
[36483.839766][T201180] #@# cm_mgr_to_sspm_command(60) cmd 0x4, arg 0x1
[36483.839780][T314508] [ FTS ] fts_mode_handler: Sense OFF!
[36483.840091][T301300] set battery thermal level = 0
[36483.840094][ T1180] cm_mgr_fb_notifier_callback-
[36483.840102][ T1180] conninfra_dev_fb_notifier_callback+
[36483.840107][ T1180] @@@@@@@@@@ Conninfra enter early POWERDOWN @@@@@@@@@@@@@@
[36483.840120][ T1180] conninfra_dev_fb_notifier_callback-
[36483.840133][ T1180] [btmtk_info] btmtk_disp_notify_cb: value[1], data[1]
[36483.840571][ T1180] mtk_disp_tdshp_stop, line: 528
[36483.840583][ T1180] mtk_disp_c3d_stop, line: 909
[36483.840615][ T1180] mtk_disp_tdshp_stop, line: 528
[36483.840622][ T1180] mtk_disp_c3d_stop, line: 909
[36483.840758][T301321] conninfra@(opfunc_pre_cal_check:1078) [opfunc_pre_cal_check] [pre_cal] bt=[1] wf=[1] status=[4]
[36483.841483][T201180] mtk_disp_tdshp_unprepare
[36483.841574][T201180] mtk_disp_c3d_unprepare, line: 948
[36483.841589][T201180] mtk_dmdp_aal_unprepare
[36483.841757][T201180] lcm_panel_vddi_disable +
[36483.841815][T201180] lcm_panel_vddi_disable regulator_is_enabled = 1
[36483.841864][T201180] lcm_panel_vddi_disable -
[36483.844009][T201180] mtk_disp_tdshp_unprepare
[36483.844028][T526722] [wlan][wlan][26722]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0xff4b] TransID[0x1dc4] SeqNo[164]
[36483.844094][T201180] mtk_disp_c3d_unprepare, line: 948
[36483.844109][T201180] mtk_dmdp_aal_unprepare
[36483.844302][T201180] [DISP]CRTC0 release input fence
[36483.844415][T201180] [DISP]CRTC0 release wakelock mtk_drm_crtc_suspend 6810
[36483.844671][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1086ms> Tput: 1296(0.001mbps) [176:2:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:291
[36483.844693][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13174,0,0,7951,7932,0,7733,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36483.845480][T225960] do_mminfra_bkrs: call scmi_tinysys_common_set(0)
[36483.845614][T225960] do_mminfra_bkrs: call scmi_tinysys_common_set(0) err=0
[36483.845635][T225960] mminfra_cg_check SMI cg still on, CG_CON0:0xfffffff8
[36483.845673][T225960] mminfra_cg_check GCE cg still on, CG_CON0:0xfffffff8 CG_CON1:0xfffdffff
[36483.845780][T225960] [cmdq] cmdq_dump_usage: hwid:0 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36483.845789][T225960] [cmdq] cmdq_dump_usage: hwid:1 suspend:0 usage:0 mbox_usage:0 wake_lock:0
[36483.845796][T225960] mtk_mminfra_pd_callback: disable clk ref_cnt=0
[36483.847202][T626723] [wlan][wlan][26723]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0xe480] TransID[0x62a7] SeqNo[165]
[36483.852785][T503042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7863 us]
[36483.852933][T503042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:1:2:0:0] C:[10:20:50:80]=[1:0:0:0:0] M:[5:10:20:50]=[1:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36483.852953][T503042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23284 3217] T[2247 2247 2247 / 10823 10823 10821 10821] R[7951 / 11777] T_R[2 0 0 0 0]  R_R[0 0 0 0] Tok[2/4096] Rfb[4150/4150] txreg[7779] rxreg[16605]
[36483.855752][T25910] [connlog] wifi_fw irq counter = 2799
[36483.855946][T401809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36483.855965][T401809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36483.858914][T103040] [wlan][wlan][3040]wlanPktTxDone:(TX INFO) TX DONE, Type[DNS] Tag[0x6cae0e00] WIDX:PID[6:2] Status[0], SeqNo: 164
[36483.866550][ T3040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x7380, TransID 0x1dc4
[36483.876973][T403040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x7381, TransID 0x62a7
[36483.995036][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36484.001896][T114626] MTK-BTIF-EXP[I]mtk_wcn_btif_dpidle_ctrl:enter deep idle
[36484.043393][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[3221 us]
[36484.104040][T503042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000004
[36484.159488][T315673] i2c_write: write failed ret(-6), maybe in standby
[36484.160693][T15673] qcom,nq-nci 3-0028:   7 > 20020401020100
[36484.172099][T415673] qcom,nq-nci 3-0028:   7 > 20020401020100
[36484.172630][T315672] qcom,nq-nci 3-0028:   3 < 400202
[36484.172829][T415672] qcom,nq-nci 3-0028:   2 < 0000
[36484.175165][T315673] qcom,nq-nci 3-0028:   4 > 20090103
[36484.175831][T15672] qcom,nq-nci 3-0028:   3 < 400901
[36484.176642][T15672] qcom,nq-nci 3-0028:   1 < 00
[36484.192441][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6428 us]
[36484.241427][ T1115] night_charging_get_flag pinfo->night_charging=0
[36484.244221][  T407] logd: logdr: UID=10189 GID=10189 PID=26732 n tail=0 logMask=4 pid=0 start=1773291292185000000ns deadline=0ns
[36484.266685][T703040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x84] SEQ[197] SET[0] LEN[72]
[36484.266712][T703040] [wlan][3040]cnmTimerStartTimer:(CNM WARN) Invalid NextExpiredSysTime: 36183720, currentSysTime: 36184228
[36484.267038][T403042] [wlan][3042]wlanTxCmdDoneCb:(TX INFO) Add command: 00000000492ca9dd, nicCmdEventQueryLinkStats.cfi_jt [wlan_drv_gen4m_6895], cmd=0x84, seq=197
[36484.269524][T703040] [wlan][3040]nicGetPendingCmdInfo:(TX INFO) Get command: 00000000492ca9dd, nicCmdEventQueryLinkStats.cfi_jt [wlan_drv_gen4m_6895], cmd=0x84, seq=197
[36484.273180][T216920] [wlan][16920]priv_support_driver_cmd:(REQ INFO) priv_support_driver_cmd: driver cmd "SETSUSPENDMODE 1" on wlan0,(00000000967bdaf4,00000000f7a0b73b)
[36484.273235][T216920] [wlan][16920]priv_driver_set_suspend_mode:(REQ INFO) priv_driver_set_suspend_mode: Set suspend mode [1]
[36484.273723][T403040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x0A] SEQ[198] SET[1] LEN[132]
[36484.274882][T103040] [wlan][3040]wlanoidSetMulticastList:(OID INFO) MCAST white list: total=1 MAC0=33:33:**:**:**:01 MAC1=00:00:**:**:**:00 MAC2=00:00:**:**:**:00 MAC3=00:00:**:**:**:00 MAC4=00:00:**:**:**:00
[36484.274914][T103040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0xC1] SEQ[199] SET[1] LEN[264]
[36484.275506][T716920] [wlan][16920]kalGetIPv4Address:(INIT INFO) IPv4 addr [0][192.***.***.41] mask [255.***.***.0]
[36484.275520][T716920] [wlan][16920]kalGetIPv6Address:(INIT INFO) IPv6 addr [0][2408:8352:0450:284c:2802:bff8:f972:b2af]
[36484.275525][T716920] [wlan][16920]kalGetIPv6Address:(INIT INFO) IPv6 addr [1][2408:8352:0450:284c:f038:2aff:fe4a:4a7c]
[36484.275529][T716920] [wlan][16920]kalGetIPv6Address:(INIT INFO) IPv6 addr [2][fe80:0000:0000:0000:f038:2aff:fe4a:4a7c]
[36484.275757][T103040] [wlan][3040]wlanoidSetNetworkAddress:(OID INFO) wlanoidSetNetworkAddress:IPv4 Addr[0][192.***.***.41]Mask[255.***.***.0]BSS[0] ver[1]
[36484.275782][T103040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x10] SEQ[200] SET[1] LEN[76]
[36484.276521][T103040] [wlan][3040]wlanoidSetIPv6NetworkAddress:(INIT INFO) wlanoidSetIPv6NetworkAddress: IPv6 Addr [0][2408:8352:0450:284c:2802:bff8:f972:b2af]BSS[0]
[36484.276541][T103040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x4B] SEQ[201] SET[1] LEN[84]
[36484.277471][T203040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x58] SEQ[202] SET[1] LEN[132]
[36484.278698][T203040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x0A] SEQ[203] SET[1] LEN[132]
[36484.279543][ T3040] [wlan][3040]wlanoidSetMulticastList:(OID INFO) MCAST white list: total=1 MAC0=33:33:**:**:**:01 MAC1=00:00:**:**:**:00 MAC2=00:00:**:**:**:00 MAC3=00:00:**:**:**:00 MAC4=00:00:**:**:**:00
[36484.279565][ T3040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0xC1] SEQ[204] SET[1] LEN[264]
[36484.280047][T303040] <FW>PWR SINFO-1: 2 ce 2 8 22c81c2 16 0 601a 0 1b 0 b2381 0 0 0 dca19cf 0 22c8020 8c 0 0 0 0 0 86 0 0 0 0 0 0 6 0 0 0 0 0 22c7c1e 4f 0 0 0 0 0 3d 0 0 0 0 0 0 7 0 0 b 0 0 22c7610 bb2 0 0 0 0 0 21f 0 0 0 0 0 0 8b 0 4 16 0 8ee 16 5a 9 39 9 36 2 c 24 93 0 0 0 0 0
[36484.280218][T316920] [wlan][16920]kalGetIPv4Address:(INIT INFO) IPv4 address is not available for dev(0x00000000d99d7582)
[36484.281134][T303040] <FW>PWR SINFO-2: 0 0 0 9 8 0 2 3 0 0 0 0 f054e14a 22c7782 f04db53a 22c7d89 f0534eb2 22c8174 0 0 0 0 0 0 0 0 0 0 f054e14a 1 f04db53a 1 f0534eb2 1 0 0 0 0 0 0 0 0 0 0 2 0 0 0 0 0 0 0 0 a 7 0 12 2 0 0 0 0 0 0 0 0 173 6 0 0 0 0 0 0 25 0 202 0 0 0 0 0 0 0 0 0 0 0
[36484.281519][T303040] <FW>PWR SINFO-3: 0 0 0 0 0 0 0 0 110 c 0 0 40 0 0 10000 91010064 10000 10000 0 1f f d 0 0 0 0 0
[36484.282289][T203040] <FW>PWR RATIO-1: 2 2998 2240 733 1 24 5000000 2 4 8 45 2
[36484.283280][T116920] [wlan][16920]kalGetIPv4Address:(INIT INFO) IPv4 address is not available for dev(0x000000006c111166)
[36484.302474][T103042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36484.408283][T403042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7551 us]
[36484.466574][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36484.623327][T400273] input_suspend_get 0
[36484.623351][T400273] input_suspend_get 0
[36484.630143][T400273] Vbat=4398 vbats=4410 vbus:5018 ibus:380 I=0 T=25 uisoc:100 type:usb>usb pd:3 swchg_ibat:0 cv:4450000
[36484.630185][T400273] input_suspend_get 0
[36484.630190][T400273] input_suspend_get 0
[36484.630194][T400273] input_suspend_get 0
[36484.630202][T400273] mtk_charger_start_timer: alarm timer start:0, 36494 588526789
[36484.638474][T200273] input_suspend_get 0
[36484.638509][T200273] input_suspend_get 0
[36484.660505][T300273] tmp:25 (jeita:0 sm:0 cv:0 en:0) (sm:1) en:1 c:0 s:0 ov:0 sc:0 1 1 saf_cmd:-1 bat_mon:1 0
[36484.668681][T300273] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5006
[36484.668733][T300273] select_cv:cv=4450
[36484.668750][T300273] charging_current_limit(uA) 1475000 1500000 300000 300000
[36484.668762][T300273] C to C set input current 1500mA charging
[36484.668935][T300273] m:0 chg1:-1,-1,1475,1500 chg2:-1,-1,0,0 dvchg1:-1 sc:1500000 -1 0 type:4:3 usb_unlimited:0 usbif:0 usbsm:0 aicl:1475000 atm:0 bm:0 b:1 mtbf:0
[36484.668957][T300273] do_algorithm is_basic:1
[36484.669115][T300273] do_algorithm:old_cv=0,cv=4450, vbat_mon_en=1
[36484.705511][T200273] mt6375-chg 11280000.i2c:mt6375@34:chg: mt6375_dump_registers CC = 1500mA, AICR = 1475mA, MIVR = 4600mV, IEOC = 200mA, CV = 4450mV
[36484.705511][T200273] VBUS = 5031mV, IBUS = 477mA, VBAT = 4410mV, IBAT = 0mA, VSYS = 4455mV
[36484.705511][T200273] CHG_STAT = 0x01, CHG_STAT0 = 0xC1, CHG_STAT1 = 0x00, CHG_TOP1 = 0xB2, CHG_TOP2 = 0x43, CHG_EOC = 0x30
[36484.707239][T200273] input_suspend_get 0
[36484.707531][T201189] [smartcharging] [sc1]en:0 t:0,80000,46871,33129 t:3600,29529,-1,-1 c:0,2000 ibus:0 uisoc:100,80 s:3000 ans:ignore
[36484.714345][T200273] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5050
[36484.782297][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5050
[36484.783981][T726018] i2c_error_count_get 0
[36484.784016][T726018] authentic_get 1
[36484.784044][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36484.784060][T726018] charge_done_get 1
[36484.784080][T726018] capacity_raw_get 9904
[36484.784096][T726018] fastcharge_mode_set 0
[36484.784110][T726018] monitor_delay_set 30000
[36484.784129][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36484.784143][T726018] capacity_raw_get 9904
[36484.784156][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36484.784166][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36484.784175][T726018] handle_step_charge index = 3
[36484.784180][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36484.784192][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36484.784202][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36484.790644][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36484.790683][T726018] connector_temp_get 243
[36484.790694][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36484.837299][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7488 us]
[36484.866524][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[2:0:3:0:0] C:[10:20:50:80]=[7:0:0:0:0] M:[5:10:20:50]=[7:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36484.866566][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23323 3218] T[2261 2261 2261 / 10828 10828 10828 10828] R[7955 / 11794] T_R[0 0 0 0 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4149/4150] txreg[7784] rxreg[16626]
[36484.866896][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1023ms> Tput: 7080(0.006mbps) [492:6:414:3][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36484.866917][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13189,0,0,7954,7936,0,7737,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36484.869762][T726018] power_debug_work: start 
[36484.869789][T726018] active wake lock : WLAN timeout,last_time:36484788
[36484.869878][T726018] active wake lock : 3-0028,last_time:36484130
[36484.869909][T726018] active wake lock : 11201000.usb0,last_time:8721
[36484.872760][ T1809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36484.872780][ T1809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36484.872881][ T1809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36484.872925][ T1809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36484.877514][  T418] type=1400 audit(1773291671.984:18913): avc: denied { read } for comm=".unixsocketdemo" name="unix" dev="proc" ino=4026532131 scontext=u:r:untrusted_app:s0:c14,c257,c512,c768 tcontext=u:object_r:proc_net:s0 tclass=file permissive=1 app=com.example.unixsocketdemo
[36484.877954][  T418] type=1400 audit(1773291671.984:18914): avc: denied { open } for comm=".unixsocketdemo" path="/proc/23673/net/unix" dev="proc" ino=4026532131 scontext=u:r:untrusted_app:s0:c14,c257,c512,c768 tcontext=u:object_r:proc_net:s0 tclass=file permissive=1 app=com.example.unixsocketdemo
[36484.878081][  T418] type=1400 audit(1773291671.984:18915): avc: denied { getattr } for comm=".unixsocketdemo" path="/proc/23673/net/unix" dev="proc" ino=4026532131 scontext=u:r:untrusted_app:s0:c14,c257,c512,c768 tcontext=u:object_r:proc_net:s0 tclass=file permissive=1 app=com.example.unixsocketdemo
[36485.150934][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36485.198002][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7713 us]
[36485.255868][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36485.421561][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7240 us]
[36485.480928][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36485.537046][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7604 us]
[36485.594707][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36485.806726][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36485.808636][T726018] i2c_error_count_get 0
[36485.808669][T726018] authentic_get 1
[36485.808688][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36485.808699][T726018] charge_done_get 1
[36485.808716][T726018] capacity_raw_get 9904
[36485.808729][T726018] fastcharge_mode_set 0
[36485.808738][T726018] monitor_delay_set 30000
[36485.808751][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36485.808759][T726018] capacity_raw_get 9904
[36485.808771][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36485.808781][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36485.808789][T726018] handle_step_charge index = 3
[36485.808794][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36485.808807][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36485.808817][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36485.816528][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36485.816565][T726018] connector_temp_get 243
[36485.816577][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36485.957643][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1090ms> Tput: 23112(0.022mbps) [2676:15:474:6][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36485.957773][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13198,0,0,7960,7941,0,7742,199,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36485.965334][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7572 us]
[36485.965537][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[9:1:5:0:0] C:[10:20:50:80]=[14:0:0:0:0] M:[5:10:20:50]=[14:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36485.965593][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23356 3220] T[2261 2261 2261 / 10843 10843 10842 10842] R[7960 / 11810] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7798] rxreg[16647]
[36485.969773][T25910] [connlog] wifi_fw irq counter = 2801
[36485.969850][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36485.969866][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36485.969972][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36485.969982][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36486.038743][T303040] [wlan][wlan][3040]cnmTimerStartTimer:(CNM INFO) [WLAN-LP] Start timer 000000005bb70dbc 200 ms -handler(qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895])
[36486.179668][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36486.205766][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7350 us]
[36486.242266][T303040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 000000005bb70dbc func qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895]
[36486.263742][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36486.400799][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[4330 us]
[36486.449205][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000001
[36486.830419][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36486.832379][T726018] i2c_error_count_get 0
[36486.832412][T726018] authentic_get 1
[36486.832436][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36486.832450][T726018] charge_done_get 1
[36486.832469][T726018] capacity_raw_get 9904
[36486.832483][T726018] fastcharge_mode_set 0
[36486.832492][T726018] monitor_delay_set 30000
[36486.832504][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36486.832512][T726018] capacity_raw_get 9904
[36486.832523][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36486.832533][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36486.832541][T726018] handle_step_charge index = 3
[36486.832545][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36486.832558][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36486.832568][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36486.838738][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36486.838775][T726018] connector_temp_get 243
[36486.838786][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36486.925688][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7241 us]
[36486.975066][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36486.975146][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36486.975256][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36486.975266][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36486.980447][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[4:0:2:0:0] C:[10:20:50:80]=[7:0:0:0:0] M:[5:10:20:50]=[7:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36486.980476][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23375 3223] T[2261 2261 2261 / 10849 10849 10849 10849] R[7965 / 11820] T_R[0 0 0 0 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4149/4150] txreg[7803] rxreg[16662]
[36486.980582][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1023ms> Tput: 4952(0.004mbps) [364:6:270:5][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36486.980591][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36486.980616][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13207,0,0,7965,7946,0,7744,202,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36487.117842][T303042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7331 us]
[36487.176646][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36487.214195][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7701 us]
[36487.271620][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36487.383868][ T1080] POWER_SUPPLY_PROP_STATUS=4
[36487.385338][T300418] type=1400 audit(1773291674.492:18916): avc: denied { search } for comm="health@2.1-serv" name="power_supply" dev="sysfs" ino=70671 scontext=u:r:hal_health_default:s0 tcontext=u:object_r:sysfs_power_supply:s0 tclass=dir permissive=1
[36487.385665][ T1080] input_suspend_get 0
[36487.385952][T300418] type=1400 audit(1773291674.492:18917): avc: denied { read } for comm="health@2.1-serv" name="type" dev="sysfs" ino=70681 scontext=u:r:hal_health_default:s0 tcontext=u:object_r:sysfs_power_supply:s0 tclass=file permissive=1
[36487.386646][T300418] type=1400 audit(1773291674.492:18918): avc: denied { open } for comm="health@2.1-serv" path="/sys/devices/virtual/power_supply/mtk-gauge/type" dev="sysfs" ino=70681 scontext=u:r:hal_health_default:s0 tcontext=u:object_r:sysfs_power_supply:s0 tclass=file permissive=1
[36487.387173][T300418] type=1400 audit(1773291674.492:18919): avc: denied { getattr } for comm="health@2.1-serv" path="/sys/devices/virtual/power_supply/mtk-gauge/type" dev="sysfs" ino=70681 scontext=u:r:hal_health_default:s0 tcontext=u:object_r:sysfs_power_supply:s0 tclass=file permissive=1
[36487.387970][ T1080] mt_usb_get_property psp:4
[36487.388015][ T1080] input_suspend_get 0
[36487.388047][ T1080] input_suspend_get 0
[36487.388513][ T1080] mt_usb_get_property psp:8
[36487.388549][ T1080] input_suspend_get 0
[36487.388920][ T1080] healthd: battery l=100 v=4398 t=25.6 h=2 st=5 c=0 chg=u
[36487.389000][ T1080] healthd: battery l=100 v=4398 t=25.6 h=2 st=5 c=0 chg=u
[36487.395828][ T1080] POWER_SUPPLY_PROP_STATUS=4
[36487.396905][ T1080] input_suspend_get 0
[36487.399010][ T1080] mt_usb_get_property psp:4
[36487.399051][ T1080] input_suspend_get 0
[36487.399080][ T1080] input_suspend_get 0
[36487.399534][ T1080] mt_usb_get_property psp:8
[36487.399571][ T1080] input_suspend_get 0
[36487.399820][ T1080] healthd: battery l=100 v=4398 t=25.6 h=2 st=5 c=0 chg=u
[36487.430521][T301115] night_charging_get_flag pinfo->night_charging=0
[36487.433303][ T1115] fastcharge_mode_get 0
[36487.439130][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7435 us]
[36487.498267][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36487.854254][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36487.856122][T726018] i2c_error_count_get 0
[36487.856154][T726018] authentic_get 1
[36487.856171][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36487.856182][T726018] charge_done_get 1
[36487.856198][T726018] capacity_raw_get 9904
[36487.856211][T726018] fastcharge_mode_set 0
[36487.856219][T726018] monitor_delay_set 30000
[36487.856233][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36487.856241][T726018] capacity_raw_get 9904
[36487.856253][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36487.856262][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36487.856271][T726018] handle_step_charge index = 3
[36487.856276][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36487.856288][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36487.856298][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36487.863678][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36487.863722][T726018] connector_temp_get 243
[36487.863737][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36488.453717][C501098] [name:spm&][SPM] system_bus wake up by  R12_SYSTIMER, timer_out = 212, r13 = 0x80001ac0, debug_flag = 0x10000000 0x3, r12 = 0x40000, r12_ext = 0x0, raw_sta = 0x0 0x0 0x0, idle_sta = 0x0, req_sta =  0x0 0x1f000000 0x1f0 0x3e3e001a 0x0 0x3fd000 0x0, cg_check_sta =0x1f0, isr = 0x0, rt_req_sta0 = 0x0 rt_req_sta1 = 0x0 rt_req_sta2 = 0xffffffff rt_req_sta3 = 0xffffffff dram_sw_con_3 = 0x0, raw_ext_sta = 0x208a55, wake_misc = 0x180030, pcm_flag = 0x2890077 0x0 0x2810077 0x2810077, req = 0x6360200,  clk_settle = 0x60fe, 
[36488.878979][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36488.880894][T726018] i2c_error_count_get 0
[36488.880923][T726018] authentic_get 1
[36488.880951][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36488.880966][T726018] charge_done_get 1
[36488.880985][T726018] capacity_raw_get 9904
[36488.881001][T726018] fastcharge_mode_set 0
[36488.881014][T726018] monitor_delay_set 30000
[36488.881033][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36488.881048][T726018] capacity_raw_get 9904
[36488.881064][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36488.881080][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36488.881093][T726018] handle_step_charge index = 3
[36488.881099][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36488.881121][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36488.881136][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36488.888860][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36488.888892][T726018] connector_temp_get 243
[36488.888903][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36489.902091][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36489.904180][T726018] i2c_error_count_get 0
[36489.904211][T726018] authentic_get 1
[36489.904235][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36489.904249][T726018] charge_done_get 1
[36489.904269][T726018] capacity_raw_get 9904
[36489.904284][T726018] fastcharge_mode_set 0
[36489.904296][T726018] monitor_delay_set 30000
[36489.904314][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36489.904325][T726018] capacity_raw_get 9904
[36489.904337][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36489.904347][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36489.904357][T726018] handle_step_charge index = 3
[36489.904361][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36489.904374][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36489.904385][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36489.910631][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36489.910663][T726018] connector_temp_get 243
[36489.910675][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36490.245898][  T417] [DLPT] imix_r==0, skip
[36490.442349][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <3462ms> Tput: 720(0.000mbps) [315:4:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36490.442418][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13210,0,0,7965,7946,0,7744,202,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36490.450450][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[8019 us]
[36490.450674][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:4:0:0] C:[10:20:50:80]=[3:0:0:0:0] M:[5:10:20:50]=[3:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36490.450730][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23385 3223] T[2261 2261 2261 / 10853 10853 10852 10852] R[7965 / 11826] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7807] rxreg[16668]
[36490.453465][T25910] [connlog] wifi_mcu cache is full.
[36490.453501][T25910] [connlog] wifi_fw irq counter = 2803
[36490.453823][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36490.453866][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36490.454088][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36490.454115][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36490.510507][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36490.592201][T403042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6795 us]
[36490.647474][T703042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36490.757797][T25910] timesync host boottime 36490716112558
[36490.900368][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7781 us]
[36490.926700][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5050
[36490.928332][T726018] i2c_error_count_get 0
[36490.928364][T726018] authentic_get 1
[36490.928391][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4397 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36490.928407][T726018] charge_done_get 1
[36490.928427][T726018] capacity_raw_get 9904
[36490.928444][T726018] fastcharge_mode_set 0
[36490.928457][T726018] monitor_delay_set 30000
[36490.928476][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36490.928488][T726018] capacity_raw_get 9904
[36490.928501][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3
[36490.928512][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3, new_index = 3
[36490.928521][T726018] handle_step_charge index = 3
[36490.928526][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36490.928540][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36490.928549][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36490.935164][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36490.935196][T726018] connector_temp_get 243
[36490.935208][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36490.959992][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36491.021541][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6941 us]
[36491.077062][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36491.217615][  T121] [wdtk] kick watchdog
[36491.278440][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7842 us]
[36491.336203][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36491.526631][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1084ms> Tput: 5728(0.005mbps) [607:7:170:2][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36491.526672][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13217,0,0,7967,7948,0,7746,202,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36491.534078][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7379 us]
[36491.534173][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[1:0:6:0:0] C:[10:20:50:80]=[7:0:0:0:0] M:[5:10:20:50]=[7:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36491.534193][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23403 3224] T[2261 2261 2261 / 10860 10860 10859 10859] R[7967 / 11837] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7813] rxreg[16681]
[36491.537680][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36491.537766][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36491.537977][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36491.538004][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36491.644848][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36491.918606][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7587 us]
[36491.950589][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5050
[36491.952802][T726018] i2c_error_count_get 0
[36491.952830][T726018] authentic_get 1
[36491.952848][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36491.952860][T726018] charge_done_get 1
[36491.952877][T726018] capacity_raw_get 9904
[36491.952890][T726018] fastcharge_mode_set 0
[36491.952899][T726018] monitor_delay_set 30000
[36491.952912][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36491.952922][T726018] capacity_raw_get 9904
[36491.952935][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36491.952944][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36491.952953][T726018] handle_step_charge index = 3
[36491.952958][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36491.952970][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36491.952981][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36491.960704][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36491.960735][T726018] connector_temp_get 243
[36491.960747][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36491.976798][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36492.452478][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7180 us]
[36492.513255][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36492.750872][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1225ms> Tput: 2192(0.002mbps) [336:4:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36492.750935][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13220,0,0,7967,7948,0,7746,202,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36492.758114][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7165 us]
[36492.758274][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[1:0:3:0:0] C:[10:20:50:80]=[4:0:0:0:0] M:[5:10:20:50]=[4:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36492.758329][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23415 3226] T[2261 2261 2261 / 10864 10864 10863 10863] R[7967 / 11844] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7817] rxreg[16688]
[36492.760418][T25910] [connlog] wifi_fw irq counter = 2805
[36492.760680][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36492.760718][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36492.760916][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36492.760942][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36492.793887][T25115] MTK-BTIF-EXP[I]mtk_wcn_btif_dpidle_ctrl:enter deep idle
[36492.817799][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36492.974111][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36492.975983][T726018] i2c_error_count_get 0
[36492.976014][T726018] authentic_get 1
[36492.976039][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36492.976054][T726018] charge_done_get 1
[36492.976073][T726018] capacity_raw_get 9904
[36492.976089][T726018] fastcharge_mode_set 0
[36492.976101][T726018] monitor_delay_set 30000
[36492.976119][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36492.976129][T726018] capacity_raw_get 9904
[36492.976140][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36492.976151][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36492.976159][T726018] handle_step_charge index = 3
[36492.976164][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36492.976177][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36492.976187][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36492.982618][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36492.982650][T726018] connector_temp_get 243
[36492.982662][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36493.061850][C400000] [name:spm&][RC] ratio, duration_ms:10240, bus26m:0%, syspll:0%, dram:0%
[36493.061895][T423900] [ccci1/net]to:clr(0:0)

[36493.061916][T423900] [ccci1/net]ccmni0(1,1), irat_MD1, rx=(4,324,488), tx=(1,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x3,0x81,0x0,0x0)
[36493.061921][T423900] [ccci1/net]to:clr(0:0)

[36493.061929][T423900] [ccci1/net]ccmni1(0,0), irat_MD1, rx=(1028,677154,488), tx=(926,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x2,0x80,0x1,0x1)
[36493.061934][T423900] [ccci1/net]to:clr(0:0)

[36493.061949][T423900] [ccci1/net]ccmni2(0,0), irat_MD1, rx=(24,6251,488), tx=(29,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x2,0x80,0x1,0x1)
[36493.062024][T423900] [ccci1/cif]total cnt=14257;rxq0 isr_cnt=2420;rxq1 isr_cnt=81;rxq2 isr_cnt=0;rxq3 isr_cnt=2;rxq4 isr_cnt=3468;rxq5 isr_cnt=8271;rxq6 isr_cnt=1;rxq7 isr_cnt=18;rxq8 isr_cnt=0;rxq9 isr_cnt=0;rxq10 isr_cnt=0;rxq11 isr_cnt=0;rxq12 isr_cnt=0;rxq13 isr_cnt=0;rxq14 isr_cnt=0;rxq15 isr_cnt=1;rxq16 isr_cnt=0;rxq17 isr_cnt=0;rxq18 isr_cnt=0;rxq19 isr_cnt=0;rxq20 isr_cnt=0;rxq21 isr_cnt=0;rxq22 isr_cnt=0;rxq23 isr_cnt=0;
[36493.317710][C606178] [ccci1/dpmaif]net txq0-3(status=0xf)[2048]:2048-404-404(0x0), 2048-551-551(0x0), 2048-0-0(0x0), 2048-1-1(0x0)
[36493.317730][C606178] [ccci1/dpmaif]Current txq pos: w/r/rel=(808,808,808)(1102,1102,1102)(0,0,0)(2,2,2), tx_busy=0,0,0,0
[36493.454714][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7597 us]
[36493.513595][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36493.573700][C501098] [name:spm&][SPM] system_bus wake up by  R12_SYSTIMER, timer_out = 212, r13 = 0x80001ac0, debug_flag = 0x10000000 0x3, r12 = 0x40000, r12_ext = 0x0, raw_sta = 0x0 0x0 0x0, idle_sta = 0x0, req_sta =  0x0 0x1f000000 0x1f0 0x202001a 0x0 0x3fd000 0x0, cg_check_sta =0x1f0, isr = 0x0, rt_req_sta0 = 0x0 rt_req_sta1 = 0x0 rt_req_sta2 = 0xffffffff rt_req_sta3 = 0xffffffff dram_sw_con_3 = 0x0, raw_ext_sta = 0x208a55, wake_misc = 0x180030, pcm_flag = 0x2890077 0x0 0x2810077 0x2810077, req = 0x6360200,  clk_settle = 0x60fe, 
[36493.613975][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7541 us]
[36493.674394][T703042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36493.766520][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1015ms> Tput: 2064(0.002mbps) [262:3:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36493.766548][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13223,0,0,7967,7948,0,7746,202,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36493.774226][T303042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7634 us]
[36493.774315][T303042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:3:0:0] C:[10:20:50:80]=[3:0:0:0:0] M:[5:10:20:50]=[3:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36493.774335][T303042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23424 3227] T[2261 2261 2261 / 10867 10867 10866 10866] R[7967 / 11850] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7820] rxreg[16694]
[36493.776460][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36493.776483][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36493.833224][T703042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36493.934486][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7646 us]
[36493.992687][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36493.998534][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36494.000429][T726018] i2c_error_count_get 0
[36494.000451][T726018] authentic_get 1
[36494.000472][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4397 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36494.000485][T726018] charge_done_get 1
[36494.000504][T726018] capacity_raw_get 9904
[36494.000517][T726018] fastcharge_mode_set 0
[36494.000527][T726018] monitor_delay_set 30000
[36494.000542][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36494.000553][T726018] capacity_raw_get 9904
[36494.000566][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3
[36494.000576][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3, new_index = 3
[36494.000585][T726018] handle_step_charge index = 3
[36494.000590][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36494.000602][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36494.000613][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36494.008342][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36494.008375][T726018] connector_temp_get 243
[36494.008391][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36494.047455][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6992 us]
[36494.086305][T200421] [ccci1/fsm]poll MD status send msg 0
[36494.096743][T14508] [ccci1/fsm]received MD status response 896f0043
[36494.097128][T300421] [ccci1/fsm]poll MD status wait done 3748
[36494.109151][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36494.230252][T200001] init: Sending signal 9 to service 'mdnsd' (pid 26610) process group...
[36494.235620][T203040] asicConnac2xFillCmdTxdInfo: 6 callbacks suppressed
[36494.235644][T203040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x0A] SEQ[211] SET[1] LEN[132]
[36494.242349][    T1] libprocessgroup: Successfully killed process cgroup uid 1020 pid 26610 in 11ms
[36494.243750][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7746 us]
[36494.244625][T200001] init: Control message: Processed ctl.stop for 'mdnsd' from pid: 1054 (/system/bin/netd)
[36494.245403][T200001] init: Service 'mdnsd' (pid 26610) received signal 9 oneshot service took 73.150002 seconds in background
[36494.246391][C324606] [wlan][wlan][25406]statsParsePktInfo:(TX INFO) <TX><IPv6> hop-by-hop packet
[36494.247275][T203040] [wlan][wlan][3040]asicConnac2xFillCmdTxdInfo:(TX INFO) TX CMD: ID[0x0A] SEQ[212] SET[1] LEN[132]
[36494.302646][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x08000000
[36494.341767][  T154] [Hang_Detect] hang_detect thread counts down 10:10, status 1.
[36494.351423][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6443 us]
[36494.410330][T703042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36494.597171][  T283] car[0,0,0,0,0] tmp:25 soc:0 uisoc:0 vbat:0 ibat:0 baton:0 algo:0 gm3:1 0 0 0 0,boot:0
[36494.598684][  T283] [fg_drv_update_daemon]ui_ht_gap:0 ui_lt_gap:0 sw_iavg:0 0 0 nafg_m:0 0 0
[36494.598715][  T283] FG daemon is disabled
[36494.598747][  T283] battery_update_routine
[36494.653830][T200273] input_suspend_get 0
[36494.653874][T200273] input_suspend_get 0
[36494.661198][T200273] Vbat=4398 vbats=4410 vbus:5062 ibus:120 I=0 T=25 uisoc:100 type:usb>usb pd:3 swchg_ibat:0 cv:4450000
[36494.661234][T200273] input_suspend_get 0
[36494.661255][T200273] input_suspend_get 0
[36494.661273][T200273] input_suspend_get 0
[36494.661298][T200273] mtk_charger_start_timer: alarm timer start:0, 36504 619616020
[36494.670219][T200273] input_suspend_get 0
[36494.670237][T200273] input_suspend_get 0
[36494.690995][  T273] tmp:25 (jeita:0 sm:0 cv:0 en:0) (sm:1) en:1 c:0 s:0 ov:0 sc:0 1 1 saf_cmd:-1 bat_mon:1 0
[36494.698236][  T273] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36494.698271][  T273] select_cv:cv=4450
[36494.698300][  T273] charging_current_limit(uA) 1475000 1500000 300000 300000
[36494.698323][  T273] C to C set input current 1500mA charging
[36494.698623][  T273] m:0 chg1:-1,-1,1475,1500 chg2:-1,-1,0,0 dvchg1:-1 sc:1500000 -1 0 type:4:3 usb_unlimited:0 usbif:0 usbsm:0 aicl:1475000 atm:0 bm:0 b:1 mtbf:0
[36494.698653][  T273] do_algorithm is_basic:1
[36494.698934][  T273] do_algorithm:old_cv=0,cv=4450, vbat_mon_en=1
[36494.735870][  T273] mt6375-chg 11280000.i2c:mt6375@34:chg: mt6375_dump_registers CC = 1500mA, AICR = 1475mA, MIVR = 4600mV, IEOC = 200mA, CV = 4450mV
[36494.735870][  T273] VBUS = 5062mV, IBUS = 132mA, VBAT = 4410mV, IBAT = 0mA, VSYS = 4456mV
[36494.735870][  T273] CHG_STAT = 0x01, CHG_STAT0 = 0xC1, CHG_STAT1 = 0x00, CHG_TOP1 = 0xB2, CHG_TOP2 = 0x43, CHG_EOC = 0x30
[36494.738045][ T1189] [smartcharging] [sc1]en:0 t:0,80000,46881,33119 t:3600,29519,-1,-1 c:0,2000 ibus:0 uisoc:100,80 s:3000 ans:ignore
[36494.738184][T200273] input_suspend_get 0
[36494.746200][  T273] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36494.854307][T726018] power_debug_work: start 
[36494.854440][T726018] active wake lock : 11201000.usb0,last_time:8721
[36494.933976][T400161] [wdk-c] cpu=4 o_k=4 lbit=0x10 cbit=0xff,ef,7,1,773528308,ff,0,0,0,0,[36494933969081,15000000] 27
[36494.934013][T200159] [wdk-c] cpu=2 o_k=2 lbit=0x14 cbit=0xff,eb,7,1,773528308,ff,0,0,0,0,[36494933981081,14999989] 27
[36494.950113][T700164] [wdk-c] cpu=7 o_k=7 lbit=0x94 cbit=0xff,6b,7,1,773528308,ff,0,0,0,0,[36494950089389,14983880] 27
[36495.022649][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36495.024266][T726018] i2c_error_count_get 0
[36495.024291][T726018] authentic_get 1
[36495.024319][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36495.024335][T726018] charge_done_get 1
[36495.024354][T726018] capacity_raw_get 9904
[36495.024371][T726018] fastcharge_mode_set 0
[36495.024384][T726018] monitor_delay_set 30000
[36495.024403][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36495.024417][T726018] capacity_raw_get 9904
[36495.024434][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36495.024450][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36495.024463][T726018] handle_step_charge index = 3
[36495.024469][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36495.024491][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36495.024506][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36495.025204][  T157] [wdk-c] cpu=0 o_k=0 lbit=0x95 cbit=0xff,6a,7,1,773528308,ff,0,0,0,0,[36495025166082,14908806] 27
[36495.026019][T300160] [wdk-c] cpu=3 o_k=3 lbit=0x9d cbit=0xff,62,7,1,773528308,ff,0,0,0,0,[36495025979235,14907991] 27
[36495.030852][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36495.030876][T726018] connector_temp_get 243
[36495.030890][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36495.046178][C300000] [wlan][wlan][0]statsParsePktInfo:(TX INFO) <TX><IPv6> hop-by-hop packet
[36495.046933][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1281ms> Tput: 3552(0.003mbps) [570:7:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:2/4096 [0:2:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36495.046989][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13228,0,0,7967,7948,0,7746,202,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36495.054718][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7731 us]
[36495.055019][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[2:0:5:0:0] C:[10:20:50:80]=[6:0:0:0:0] M:[5:10:20:50]=[6:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36495.055075][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23441 3228] T[2263 2263 2263 / 10874 10874 10872 10872] R[7967 / 11861] T_R[2 0 0 0 0]  R_R[0 0 0 0] Tok[2/4096] Rfb[4150/4150] txreg[7826] rxreg[16705]
[36495.058827][T25910] [connlog] wifi_fw irq counter = 2807
[36495.059086][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36495.059121][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36495.059332][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36495.059359][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36495.114810][T703042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36495.245762][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7320 us]
[36495.304999][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000040
[36495.374856][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7791 us]
[36495.433420][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36495.470494][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7605 us]
[36495.528283][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36495.669928][T25115] MTK-BTIF-EXP[I]mtk_wcn_btif_dpidle_ctrl:enter deep idle
[36495.782929][ T1870] hang_detect HANG_KICK ( 300)
[36495.782963][ T1870] [Hang_Detect] hang_detect enabled 10
[36495.789355][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6457 us]
[36495.799934][T303040] [wlan][wlan][3040]cnmTimerStartTimer:(CNM INFO) [WLAN-LP] Start timer 000000005bb70dbc 200 ms -handler(qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895])
[36495.848528][T703042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36496.002165][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 000000005bb70dbc func qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895]
[36496.014810][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7659 us]
[36496.046114][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5043
[36496.047977][T726018] i2c_error_count_get 0
[36496.048002][T726018] authentic_get 1
[36496.048027][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36496.048041][T726018] charge_done_get 1
[36496.048060][T726018] capacity_raw_get 9904
[36496.048075][T726018] fastcharge_mode_set 0
[36496.048086][T726018] monitor_delay_set 30000
[36496.048105][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36496.048118][T726018] capacity_raw_get 9904
[36496.048133][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36496.048146][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36496.048155][T726018] handle_step_charge index = 3
[36496.048159][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36496.048172][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36496.048181][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36496.054748][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36496.054777][T726018] connector_temp_get 243
[36496.054790][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36496.059457][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36496.059493][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36496.059701][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36496.059728][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36496.069026][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[1:0:5:0:0] C:[10:20:50:80]=[8:0:0:0:0] M:[5:10:20:50]=[8:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36496.069053][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23462 3231] T[2263 2263 2263 / 10880 10880 10880 10880] R[7970 / 11874] T_R[0 0 0 0 0]  R_R[0 0 0 0] Tok[0/4096] Rfb[4149/4150] txreg[7832] rxreg[16720]
[36496.069146][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1022ms> Tput: 5544(0.005mbps) [456:6:253:3][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36496.069159][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36496.069176][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13237,0,0,7970,7951,0,7747,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36497.070920][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7776 us]
[36497.070935][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36497.071166][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:1:0:0] C:[10:20:50:80]=[0:0:0:0:0] M:[5:10:20:50]=[0:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36497.071223][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23463 3231] T[2263 2263 2263 / 10881 10881 10880 10880] R[7970 / 11874] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7833] rxreg[16720]
[36497.072003][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1003ms> Tput: 744(0.000mbps) [94:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36497.072060][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13237,0,0,7970,7951,0,7747,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36497.072798][T726018] i2c_error_count_get 0
[36497.072820][T726018] authentic_get 1
[36497.072844][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36497.072859][T726018] charge_done_get 1
[36497.072878][T726018] capacity_raw_get 9904
[36497.072893][T726018] fastcharge_mode_set 0
[36497.072906][T726018] monitor_delay_set 30000
[36497.072922][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36497.072936][T726018] capacity_raw_get 9904
[36497.072949][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36497.072959][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36497.072967][T726018] handle_step_charge index = 3
[36497.072972][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36497.072985][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36497.072995][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36497.073184][T25910] [connlog] wifi_fw irq counter = 2809
[36497.073257][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36497.073287][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36497.080748][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 244
[36497.080774][T726018] connector_temp_get 244
[36497.080789][T726018] monitor_typec_burn get typec temp=244 otg_enable=0
[36497.132278][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36497.390090][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7439 us]
[36497.450146][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36497.670647][ T3040] [wlan][3040]wlanDumpBssStatistics:(SW4 INFO) LLS BSS[0] BE: T[010841] R[000000] T_D[000000] T_F[000000]
[36497.670693][ T3040] [wlan][3040]wlanDumpBssStatistics:(SW4 INFO) LLS BSS[0] BK: T[000000] R[000000] T_D[000000] T_F[000000]
[36497.670720][ T3040] [wlan][3040]wlanDumpBssStatistics:(SW4 INFO) LLS BSS[0] VI: T[000000] R[000000] T_D[000000] T_F[000000]
[36497.670745][ T3040] [wlan][3040]wlanDumpBssStatistics:(SW4 INFO) LLS BSS[0] VO: T[000042] R[000000] T_D[000000] T_F[000000]
[36497.678779][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7633 us]
[36497.737869][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36498.094574][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36498.096466][T726018] i2c_error_count_get 0
[36498.096492][T726018] authentic_get 1
[36498.096516][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36498.096531][T726018] charge_done_get 1
[36498.096550][T726018] capacity_raw_get 9904
[36498.096564][T726018] fastcharge_mode_set 0
[36498.096576][T726018] monitor_delay_set 30000
[36498.096592][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36498.096602][T726018] capacity_raw_get 9904
[36498.096613][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36498.096623][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36498.096632][T726018] handle_step_charge index = 3
[36498.096637][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36498.096650][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36498.096659][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36498.104007][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36498.104045][T726018] connector_temp_get 243
[36498.104061][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36498.182576][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1110ms> Tput: 1744(0.001mbps) [242:3:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36498.182607][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13240,0,0,7970,7951,0,7747,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36498.190338][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7648 us]
[36498.190431][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:3:0:0] C:[10:20:50:80]=[3:0:0:0:0] M:[5:10:20:50]=[3:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36498.190451][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23472 3233] T[2263 2263 2263 / 10884 10884 10883 10883] R[7970 / 11880] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7836] rxreg[16726]
[36498.192388][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36498.192411][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36498.251779][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36498.693694][C501098] [name:spm&][SPM] system_bus didn't enter MCUSYS off, MCUSYS cnt is no update
[36499.118822][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36499.120882][T726018] i2c_error_count_get 0
[36499.120905][T726018] authentic_get 1
[36499.120932][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36499.120948][T726018] charge_done_get 1
[36499.120968][T726018] capacity_raw_get 9904
[36499.120985][T726018] fastcharge_mode_set 0
[36499.120998][T726018] monitor_delay_set 30000
[36499.121017][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36499.121031][T726018] capacity_raw_get 9904
[36499.121047][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36499.121062][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36499.121075][T726018] handle_step_charge index = 3
[36499.121082][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36499.121105][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36499.121119][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36499.126650][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36499.126678][T726018] connector_temp_get 243
[36499.126690][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36499.206755][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1024ms> Tput: 728(0.000mbps) [94:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36499.206813][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13241,0,0,7970,7951,0,7747,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36499.214597][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7762 us]
[36499.214826][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:1:0:0] C:[10:20:50:80]=[1:0:0:0:0] M:[5:10:20:50]=[1:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36499.214881][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23475 3234] T[2263 2263 2263 / 10885 10885 10884 10884] R[7970 / 11882] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7837] rxreg[16728]
[36499.216988][T25910] [connlog] wifi_fw irq counter = 2811
[36499.217308][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36499.217342][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36499.274739][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36499.470595][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7778 us]
[36499.530039][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36499.725838][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7286 us]
[36499.784722][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36499.982497][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7599 us]
[36500.040935][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36500.142754][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36500.144653][T726018] i2c_error_count_get 0
[36500.144677][T726018] authentic_get 1
[36500.144702][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36500.144717][T726018] charge_done_get 1
[36500.144735][T726018] capacity_raw_get 9904
[36500.144750][T726018] fastcharge_mode_set 0
[36500.144763][T726018] monitor_delay_set 30000
[36500.144780][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36500.144793][T726018] capacity_raw_get 9904
[36500.144806][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36500.144816][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36500.144825][T726018] handle_step_charge index = 3
[36500.144830][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36500.144843][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36500.144853][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36500.152215][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36500.152248][T726018] connector_temp_get 243
[36500.152265][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36500.223233][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36500.223267][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36500.223480][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36500.223507][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36500.486307][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1280ms> Tput: 2896(0.002mbps) [464:6:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:292
[36500.486336][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13245,0,0,7970,7951,0,7747,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36500.493666][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7259 us]
[36500.493776][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:6:0:0] C:[10:20:50:80]=[6:0:0:0:0] M:[5:10:20:50]=[6:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36500.493798][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23487 3237] T[2263 2263 2263 / 10891 10891 10890 10890] R[7970 / 11890] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7841] rxreg[16736]
[36500.552457][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36500.998292][T25910] timesync host boottime 36500956605482
[36501.166479][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36501.168778][T726018] i2c_error_count_get 0
[36501.168803][T726018] authentic_get 1
[36501.168827][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36501.168842][T726018] charge_done_get 1
[36501.168861][T726018] capacity_raw_get 9904
[36501.168876][T726018] fastcharge_mode_set 0
[36501.168889][T726018] monitor_delay_set 30000
[36501.168906][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36501.168919][T726018] capacity_raw_get 9904
[36501.168933][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36501.168943][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36501.168952][T726018] handle_step_charge index = 3
[36501.168958][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36501.168970][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36501.168981][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36501.176407][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36501.176443][T726018] connector_temp_get 243
[36501.176459][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36501.261845][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7242 us]
[36501.264085][T25910] [connlog] wifi_mcu cache is full.
[36501.264104][T25910] [connlog] wifi_fw irq counter = 2813
[36501.264342][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36501.264364][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36501.321640][T703042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36501.327118][T426829] [wlan][wlan][26829]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0x3ba8] TransID[0x0ec6] SeqNo[166]
[36501.327148][T426829] [wlan][wlan][26829]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0x84b8] TransID[0x2ebc] SeqNo[167]
[36501.328720][T426831] [wlan][wlan][26831]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0x6c01] TransID[0xf82c] SeqNo[168]
[36501.329105][T426832] [wlan][wlan][26832]statsParseUDPInfo:(TX INFO) <TX> DNS: IPID[0xf0d4] TransID[0x19f3] SeqNo[169]
[36501.331799][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[4365 us]
[36501.336265][T303040] [wlan][wlan][3040]wlanPktTxDone:(TX INFO) TX DONE, Type[DNS] Tag[0x6c372800] WIDX:PID[6:3] Status[0], SeqNo: 166
[36501.350218][ T3040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x791e, TransID 0x0ec6
[36501.350410][ T3040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x791f, TransID 0x2ebc
[36501.356497][ T3040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x7920, TransID 0xf82c
[36501.356655][ T3040] [wlan][wlan][3040]statsParseUDPInfo:(RX INFO) <RX> DNS: IPID 0x7921, TransID 0x19f3
[36501.456131][T303040] [wlan][wlan][3040]cnmTimerStartTimer:(CNM INFO) [WLAN-LP] Start timer 000000005bb70dbc 200 ms -handler(qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895])
[36501.486670][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1000ms> Tput: 65544(0.064mbps) [2144:20:6049:17][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36501.486731][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13265,0,0,7987,7968,0,7764,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36501.510918][T303042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[16:4:1:0:0] C:[10:20:50:80]=[21:0:0:0:0] M:[5:10:20:50]=[21:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36501.511026][T303042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23529 3239] T[2263 2263 2263 / 10912 10912 10911 10911] R[7987 / 11910] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7857] rxreg[16769]
[36501.659467][T203040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 000000005bb70dbc func qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895]
[36501.698157][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36501.824829][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7165 us]
[36502.088952][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36502.190707][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36502.192631][T726018] i2c_error_count_get 0
[36502.192660][T726018] authentic_get 1
[36502.192685][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36502.192701][T726018] charge_done_get 1
[36502.192721][T726018] capacity_raw_get 9904
[36502.192738][T726018] fastcharge_mode_set 0
[36502.192751][T726018] monitor_delay_set 30000
[36502.192771][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36502.192785][T726018] capacity_raw_get 9904
[36502.192800][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36502.192810][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36502.192819][T726018] handle_step_charge index = 3
[36502.192824][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36502.192838][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36502.192848][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36502.198648][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36502.198678][T726018] connector_temp_get 243
[36502.198690][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36502.382785][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7816 us]
[36502.387132][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36502.387165][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36502.387374][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36502.387401][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36502.387481][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36502.387506][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36502.539432][T726018] [XMCHG_BQ27Z561] read FG TBAT = 2986
[36502.540890][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1055ms> Tput: 43544(0.042mbps) [3280:23:2463:22][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36502.540935][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13289,0,0,8009,7990,0,7786,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36502.540977][T703042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[19:0:3:0:0] C:[10:20:50:80]=[22:0:0:0:0] M:[5:10:20:50]=[22:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36502.540991][T703042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23583 3240] T[2263 2263 2263 / 10934 10934 10933 10933] R[8009 / 11931] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7876] rxreg[16807]
[36502.541027][T726018] fg_update_status fg_update rsoc=100, raw_soc=9904, vbat=4398, cycle_count=748
[36502.541055][T726018] POWER_SUPPLY_PROP_STATUS=4
[36502.541060][T726018] smooth_new:sys_soc:100 last_sys_soc:100 soc_delta:0 charging_status:4 unit_time:10000 batt_ma_avg=0
[36502.541063][T726018] [XMCHG_BQ27Z561] [FG_STATUS] [UISOC RSOC RAWSOC TEMP_SOC SOH] = [100 100 9904 0 83], [VBAT CELL0 CELL1 IBAT TBAT FC FAST_MODE] = [4398 4398 4398 0 256 1 0]
[36502.747930][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36503.214546][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36503.216588][T726018] i2c_error_count_get 0
[36503.216614][T726018] authentic_get 1
[36503.216640][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36503.216655][T726018] charge_done_get 1
[36503.216674][T726018] capacity_raw_get 9904
[36503.216690][T726018] fastcharge_mode_set 0
[36503.216702][T726018] monitor_delay_set 30000
[36503.216719][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36503.216732][T726018] capacity_raw_get 9904
[36503.216747][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36503.216758][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36503.216767][T726018] handle_step_charge index = 3
[36503.216771][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36503.216784][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36503.216793][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36503.224498][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36503.224528][T726018] connector_temp_get 243
[36503.224539][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36503.301864][C400000] [name:spm&][RC] ratio, duration_ms:10240, bus26m:0%, syspll:0%, dram:0%
[36503.301904][T423900] [ccci1/net]to:clr(0:0)

[36503.301919][T423900] [ccci1/net]ccmni0(1,1), irat_MD1, rx=(4,324,488), tx=(1,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x3,0x81,0x0,0x0)
[36503.301923][T423900] [ccci1/net]to:clr(0:0)

[36503.301929][T423900] [ccci1/net]ccmni1(0,0), irat_MD1, rx=(1028,677154,488), tx=(926,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x2,0x80,0x1,0x1)
[36503.301933][T423900] [ccci1/net]to:clr(0:0)

[36503.301938][T423900] [ccci1/net]ccmni2(0,0), irat_MD1, rx=(24,6251,488), tx=(29,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x2,0x80,0x1,0x1)
[36503.302009][T423900] [ccci1/cif]total cnt=14259;rxq0 isr_cnt=2421;rxq1 isr_cnt=81;rxq2 isr_cnt=0;rxq3 isr_cnt=2;rxq4 isr_cnt=3468;rxq5 isr_cnt=8272;rxq6 isr_cnt=1;rxq7 isr_cnt=18;rxq8 isr_cnt=0;rxq9 isr_cnt=0;rxq10 isr_cnt=0;rxq11 isr_cnt=0;rxq12 isr_cnt=0;rxq13 isr_cnt=0;rxq14 isr_cnt=0;rxq15 isr_cnt=1;rxq16 isr_cnt=0;rxq17 isr_cnt=0;rxq18 isr_cnt=0;rxq19 isr_cnt=0;rxq20 isr_cnt=0;rxq21 isr_cnt=0;rxq22 isr_cnt=0;rxq23 isr_cnt=0;
[36503.407602][T25910] [connlog] wifi_fw irq counter = 2815
[36503.407860][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36503.407885][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36503.407986][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36503.407999][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36503.557696][C606178] [ccci1/dpmaif]net txq0-3(status=0xf)[2048]:2048-404-404(0x0), 2048-551-551(0x0), 2048-0-0(0x0), 2048-1-1(0x0)
[36503.557708][C606178] [ccci1/dpmaif]Current txq pos: w/r/rel=(808,808,808)(1102,1102,1102)(0,0,0)(2,2,2), tx_busy=0,0,0,0
[36503.558682][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1017ms> Tput: 1744(0.001mbps) [222:3:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:2/4096 [0:2:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36503.558740][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13290,0,0,8009,7990,0,7786,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36503.566283][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7673 us]
[36503.566551][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[1:0:2:0:0] C:[10:20:50:80]=[2:0:0:0:0] M:[5:10:20:50]=[2:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36503.566598][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23589 3242] T[2263 2263 2263 / 10937 10937 10935 10935] R[8009 / 11934] T_R[2 0 0 0 0]  R_R[0 0 0 0] Tok[2/4096] Rfb[4150/4150] txreg[7878] rxreg[16810]
[36503.572584][T303040] [wlan][wlan][3040]cnmTimerStartTimer:(CNM INFO) [WLAN-LP] Start timer 000000005bb70dbc 200 ms -handler(qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895])
[36503.624827][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36503.775181][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 000000005bb70dbc func qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895]
[36503.813691][C501098] [name:spm&][SPM] system_bus didn't enter MCUSYS off, MCUSYS cnt is no update
[36504.237747][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36504.239674][T726018] i2c_error_count_get 0
[36504.239696][T726018] authentic_get 1
[36504.239721][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36504.239736][T726018] charge_done_get 1
[36504.239754][T726018] capacity_raw_get 9904
[36504.239770][T726018] fastcharge_mode_set 0
[36504.239782][T726018] monitor_delay_set 30000
[36504.239800][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36504.239813][T726018] capacity_raw_get 9904
[36504.239827][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36504.239838][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36504.239847][T726018] handle_step_charge index = 3
[36504.239852][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36504.239865][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36504.239875][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36504.246298][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36504.246331][T726018] connector_temp_get 243
[36504.246346][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36504.398579][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7597 us]
[36504.408086][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36504.408120][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36504.430484][T203040] [wlan][wlan][3040]cnmTimerStartTimer:(CNM INFO) [WLAN-LP] Start timer 000000005bb70dbc 200 ms -handler(qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895])
[36504.457979][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36504.634230][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1076ms> Tput: 1040(0.001mbps) [74:1:66:1][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36504.634286][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13294,0,0,8010,7992,0,7786,205,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36504.635487][T203040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 000000005bb70dbc func qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895]
[36504.686185][  T273] input_suspend_get 0
[36504.686226][  T273] input_suspend_get 0
[36504.691871][  T273] Vbat=4398 vbats=4410 vbus:5062 ibus:127 I=0 T=25 uisoc:100 type:usb>usb pd:3 swchg_ibat:0 cv:4450000
[36504.691915][  T273] input_suspend_get 0
[36504.691944][  T273] input_suspend_get 0
[36504.691970][  T273] input_suspend_get 0
[36504.691999][  T273] mtk_charger_start_timer: alarm timer start:0, 36514 650315944
[36504.699925][  T273] input_suspend_get 0
[36504.699963][  T273] input_suspend_get 0
[36504.720003][  T273] tmp:25 (jeita:0 sm:0 cv:0 en:0) (sm:1) en:1 c:0 s:0 ov:0 sc:0 1 1 saf_cmd:-1 bat_mon:1 0
[36504.726475][  T273] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36504.726511][  T273] select_cv:cv=4450
[36504.726539][  T273] charging_current_limit(uA) 1475000 1500000 300000 300000
[36504.726562][  T273] C to C set input current 1500mA charging
[36504.726854][  T273] m:0 chg1:-1,-1,1475,1500 chg2:-1,-1,0,0 dvchg1:-1 sc:1500000 -1 0 type:4:3 usb_unlimited:0 usbif:0 usbsm:0 aicl:1475000 atm:0 bm:0 b:1 mtbf:0
[36504.726885][  T273] do_algorithm is_basic:1
[36504.727155][  T273] do_algorithm:old_cv=0,cv=4450, vbat_mon_en=1
[36504.761430][  T273] mt6375-chg 11280000.i2c:mt6375@34:chg: mt6375_dump_registers CC = 1500mA, AICR = 1475mA, MIVR = 4600mV, IEOC = 200mA, CV = 4450mV
[36504.761430][  T273] VBUS = 5062mV, IBUS = 122mA, VBAT = 4410mV, IBAT = 0mA, VSYS = 4455mV
[36504.761430][  T273] CHG_STAT = 0x01, CHG_STAT0 = 0xC1, CHG_STAT1 = 0x00, CHG_TOP1 = 0xB2, CHG_TOP2 = 0x43, CHG_EOC = 0x30
[36504.763306][ T1189] [smartcharging] [sc1]en:0 t:0,80000,46891,33109 t:3600,29509,-1,-1 c:0,2000 ibus:0 uisoc:100,80 s:3000 ans:ignore
[36504.763427][  T273] input_suspend_get 0
[36504.770489][  T273] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36504.838066][T726018] power_debug_work: start 
[36504.838103][T726018] active wake lock : WLAN timeout,last_time:36504593
[36504.838221][T726018] active wake lock : 11201000.usb0,last_time:8721
[36505.121679][C501098] rcu: INFO: rcu_preempt self-detected stall on CPU
[36505.121688][C501098] rcu: 	5-...!: (5249 ticks this GP) idle=1f2/1/0x4000000000000002 softirq=93408/93408 fqs=0 last_accelerate: 0803/1c87 dyntick_enabled: 1
[36505.121693][C501098] 	(t=5250 jiffies g=544633 q=9555)
[36505.121697][C501098] rcu: rcu_preempt kthread starved for 5250 jiffies! g544633 f0x0 RCU_GP_WAIT_FQS(5) ->state=0x0 ->cpu=5
[36505.121700][C501098] rcu: 	Unless rcu_preempt kthread gets sufficient CPU time, OOM is now expected behavior.
[36505.121702][C501098] rcu: RCU grace-period kthread stack dump:
[36505.121706][C501098] task:rcu_preempt     state:R
[36505.121708][C501098]   running task    
[36505.121712][C501098]  stack:    0 pid:   13 ppid:     2 flags:0x00000008
[36505.121718][C501098] Call trace:
[36505.121731][C501098]  __switch_to+0x244/0x4e4
[36505.121742][C501098]  __schedule+0x5b4/0xbe8
[36505.121746][C501098]  schedule+0x80/0x160
[36505.121750][C501098]  schedule_timeout+0x98/0x144
[36505.121758][C501098]  rcu_gp_fqs_loop+0x154/0xac4
[36505.121761][C501098]  rcu_gp_kthread+0x68/0x3e4
[36505.121769][C501098]  kthread+0x150/0x200
[36505.121775][C501098]  ret_from_fork+0x10/0x30
[36505.121779][C501098] Task dump for CPU 5:
[36505.121782][C501098] task:android.hardwar state:R
[36505.121783][C501098]   running task    
[36505.121786][C501098]  stack:    0 pid: 1098 ppid:     1 flags:0x04000002
[36505.121789][C501098] Call trace:
[36505.121798][C501098]  dump_backtrace.cfi_jt+0x0/0x8
[36505.121803][C501098]  sched_show_task+0x198/0x214
[36505.121807][C501098]  rcu_dump_cpu_stacks+0x220/0x32c
[36505.121810][C501098]  print_cpu_stall+0x258/0x650
[36505.121812][C501098]  check_cpu_stall+0x154/0x450
[36505.121815][C501098]  rcu_sched_clock_irq+0xec/0x5a8
[36505.121820][C501098]  update_process_times+0xcc/0x15c
[36505.121827][C501098]  tick_sched_timer+0x16c/0x388
[36505.121832][C501098]  __run_hrtimer+0x134/0x648
[36505.121835][C501098]  hrtimer_interrupt+0x1d4/0x680
[36505.121844][C501098]  arch_timer_handler_virt+0x5c/0x9c
[36505.121848][C501098]  handle_percpu_devid_irq+0xc0/0x4cc
[36505.121855][C501098]  __handle_domain_irq+0x114/0x1e4
[36505.121861][C501098]  gic_handle_irq.30283+0x6c/0x2b8
[36505.121863][C501098]  el1_irq+0xe4/0x1c0
[36505.121871][C501098]  queued_spin_lock_slowpath+0x88/0x3c0
[36505.121880][C501098]  0xffffffe42d001b40
[36505.121882][C501098]  0xffffffe42cd04680
[36505.121886][C501098]  do_translation_fault+0x48/0x64
[36505.121892][C501098]  do_mem_abort+0x6c/0x164
[36505.121896][C501098]  el1_abort+0x44/0x68
[36505.121899][C501098]  el1_sync_handler+0x58/0x88
[36505.121901][C501098]  el1_sync+0x8c/0x140
[36505.121907][C501098]  copy_from_kernel_nofault+0x54/0x1b4
[36505.121923][C501098]  get_dmabuf_from_file+0x48/0xfc [mtk_heap_debug]
[36505.121928][C501098]  dmabuf_rbtree_add_all_pid+0x14c/0x890 [mtk_heap_debug]
[36505.121932][C501098]  dmabuf_rbtree_add_all+0x94/0x260 [mtk_heap_debug]
[36505.121936][C501098]  dmabuf_rbtree_dump_all+0x64/0x72c [mtk_heap_debug]
[36505.121940][C501098]  heap_stat_pid_proc_show+0x38/0x54 [mtk_heap_debug]
[36505.121947][C501098]  seq_read_iter+0x19c/0x75c
[36505.121950][C501098]  seq_read+0xfc/0x134
[36505.121956][C501098]  proc_reg_read+0xec/0x20c
[36505.121962][C501098]  vfs_read+0xf4/0x368
[36505.121965][C501098]  ksys_read+0x7c/0x150
[36505.121968][C501098]  __arm64_sys_read+0x20/0x30
[36505.121973][C501098]  el0_svc_common+0xd4/0x270
[36505.121976][C501098]  el0_svc+0x28/0x88
[36505.121978][C501098]  el0_sync_handler+0x8c/0xf0
[36505.121981][C501098]  el0_sync+0x1b4/0x1c0
[36505.263121][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36505.265057][T726018] i2c_error_count_get 0
[36505.265080][T726018] authentic_get 1
[36505.265106][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36505.265122][T726018] charge_done_get 1
[36505.265141][T726018] capacity_raw_get 9904
[36505.265157][T726018] fastcharge_mode_set 0
[36505.265170][T726018] monitor_delay_set 30000
[36505.265189][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36505.265203][T726018] capacity_raw_get 9904
[36505.265217][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36505.265228][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36505.265236][T726018] handle_step_charge index = 3
[36505.265240][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36505.265253][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36505.265263][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36505.273142][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36505.273175][T726018] connector_temp_get 243
[36505.273190][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36505.581634][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7236 us]
[36505.581832][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:3:0:0] C:[10:20:50:80]=[3:0:0:0:0] M:[5:10:20:50]=[3:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36505.581858][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23597 3243] T[2263 2263 2263 / 10940 10940 10938 10938] R[8011 / 11938] T_R[2 0 0 0 0]  R_R[0 0 0 0] Tok[2/4096] Rfb[4150/4150] txreg[7880] rxreg[16816]
[36505.583395][T25910] [connlog] wifi_fw irq counter = 2817
[36505.583492][ T1809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36505.583503][ T1809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36505.640511][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1006ms> Tput: 1696(0.001mbps) [148:2:66:1][0:0:0:0][0:0:0:0][0:0:0:0] Pending:0/4096 [0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36505.640540][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13295,0,0,8011,7992,0,7786,206,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36505.640582][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36505.871299][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7752 us]
[36505.930447][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36506.286330][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36506.288172][T726018] i2c_error_count_get 0
[36506.288194][T726018] authentic_get 1
[36506.288212][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4397 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36506.288223][T726018] charge_done_get 1
[36506.288238][T726018] capacity_raw_get 9904
[36506.288251][T726018] fastcharge_mode_set 0
[36506.288260][T726018] monitor_delay_set 30000
[36506.288272][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36506.288281][T726018] capacity_raw_get 9904
[36506.288293][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3
[36506.288304][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3, new_index = 3
[36506.288312][T726018] handle_step_charge index = 3
[36506.288318][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36506.288332][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36506.288342][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36506.296038][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36506.296066][T726018] connector_temp_get 242
[36506.296078][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36506.381607][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7238 us]
[36506.440580][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36506.717769][  T121] [wdtk] kick watchdog
[36507.311008][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36507.312960][T726018] i2c_error_count_get 0
[36507.312985][T726018] authentic_get 1
[36507.313012][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4397 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36507.313028][T726018] charge_done_get 1
[36507.313047][T726018] capacity_raw_get 9904
[36507.313065][T726018] fastcharge_mode_set 0
[36507.313078][T726018] monitor_delay_set 30000
[36507.313097][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36507.313111][T726018] capacity_raw_get 9904
[36507.313127][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3
[36507.313143][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3, new_index = 3
[36507.313157][T726018] handle_step_charge index = 3
[36507.313163][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36507.313181][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36507.313193][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36507.320965][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36507.320994][T726018] connector_temp_get 242
[36507.321006][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36507.495143][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36507.495178][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36507.910550][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <2270ms> Tput: 920(0.000mbps) [262:3:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36507.910580][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13297,0,0,8011,7992,0,7786,206,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36507.918084][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7423 us]
[36507.918178][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:3:0:0] C:[10:20:50:80]=[4:0:0:0:0] M:[5:10:20:50]=[4:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36507.918201][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23606 3246] T[2263 2263 2263 / 10943 10943 10942 10942] R[8011 / 11944] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7883] rxreg[16822]
[36507.977003][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36508.177946][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[11156 us]
[36508.234733][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36508.334793][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36508.336570][T726018] i2c_error_count_get 0
[36508.336595][T726018] authentic_get 1
[36508.336623][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36508.336638][T726018] charge_done_get 1
[36508.336659][T726018] capacity_raw_get 9904
[36508.336675][T726018] fastcharge_mode_set 0
[36508.336688][T726018] monitor_delay_set 30000
[36508.336707][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36508.336722][T726018] capacity_raw_get 9904
[36508.336737][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36508.336748][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36508.336757][T726018] handle_step_charge index = 3
[36508.336761][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36508.336773][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36508.336783][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36508.343051][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 243
[36508.343080][T726018] connector_temp_get 243
[36508.343092][T726018] monitor_typec_burn get typec temp=243 otg_enable=0
[36508.429485][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6881 us]
[36508.459137][T303040] [wlan][wlan][3040]cnmTimerStartTimer:(CNM INFO) [WLAN-LP] Start timer 000000005bb70dbc 200 ms -handler(qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895])
[36508.483057][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36508.662196][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 000000005bb70dbc func qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895]
[36508.933692][C501098] [name:spm&][SPM] system_bus didn't enter MCUSYS off, MCUSYS cnt is no update
[36509.190523][  T421] [ccci1/fsm]poll MD status send msg 0
[36509.200911][T14508] [ccci1/fsm]received MD status response 89700043
[36509.201241][T200421] [ccci1/fsm]poll MD status wait done 3748
[36509.357663][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5031
[36509.359491][T726018] i2c_error_count_get 0
[36509.359513][T726018] authentic_get 1
[36509.359537][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36509.359551][T726018] charge_done_get 1
[36509.359569][T726018] capacity_raw_get 9904
[36509.359583][T726018] fastcharge_mode_set 0
[36509.359594][T726018] monitor_delay_set 30000
[36509.359609][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36509.359620][T726018] capacity_raw_get 9904
[36509.359631][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36509.359641][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36509.359649][T726018] handle_step_charge index = 3
[36509.359654][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36509.359665][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36509.359674][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36509.366340][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36509.366366][T726018] connector_temp_get 242
[36509.366377][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36509.446611][T403040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1536ms> Tput: 1600(0.001mbps) [242:3:66:1][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36509.446626][T403040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13301,0,0,8012,7993,0,7786,207,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36509.454175][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7432 us]
[36509.454321][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:2:1:0] C:[10:20:50:80]=[3:0:0:0:0] M:[5:10:20:50]=[3:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36509.454342][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23616 3246] T[2263 2263 2263 / 10946 10946 10945 10945] R[8012 / 11950] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7886] rxreg[16829]
[36509.457362][T25910] [connlog] wifi_fw irq counter = 2819
[36509.457639][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36509.457661][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36509.457825][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36509.457842][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36509.514432][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36509.710825][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7739 us]
[36509.769539][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36509.941887][T400161] [wdk-c] cpu=4 o_k=4 lbit=0x9d cbit=0xff,62,7,1,773528308,ff,0,0,0,0,[36509941879963,15000000] 27
[36509.941899][T400161] kick=0x9d,check=0xff
[36509.941911][T400161] SYST0 CON3 VAL19ca0
[36509.942157][T700164] [wdk-c] cpu=7 o_k=7 lbit=0x9d cbit=0xff,62,7,1,773528308,ff,0,0,0,0,[36509942131886,15000000] 27
[36509.977964][T300160] [wdk-c] cpu=3 o_k=3 lbit=0x9d cbit=0xff,62,7,1,773528308,ff,0,0,0,0,[36509977948887,15000000] 27
[36510.033789][  T157] [wdk-c] cpu=0 o_k=0 lbit=0x9d cbit=0xff,62,7,1,773528308,ff,0,0,0,0,[36510033774117,15000000] 27
[36510.033802][T200159] [wdk-c] cpu=2 o_k=2 lbit=0x9d cbit=0xff,62,7,1,773528308,ff,0,0,0,0,[36510033786117,15000000] 27
[36510.382472][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36510.384571][T726018] i2c_error_count_get 0
[36510.384596][T726018] authentic_get 1
[36510.384622][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36510.384636][T726018] charge_done_get 1
[36510.384655][T726018] capacity_raw_get 9904
[36510.384670][T726018] fastcharge_mode_set 0
[36510.384682][T726018] monitor_delay_set 30000
[36510.384699][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36510.384711][T726018] capacity_raw_get 9904
[36510.384724][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36510.384734][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36510.384743][T726018] handle_step_charge index = 3
[36510.384747][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36510.384760][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36510.384770][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36510.392456][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36510.392486][T726018] connector_temp_get 242
[36510.392498][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36510.725887][T225960] rcu: INFO: rcu_preempt detected expedited stalls on CPUs/tasks: {
[36510.725900][T225960]  5-...
[36510.725912][T225960]  6-...
[36510.725921][T225960]  } 5256 jiffies s: 5309 root: 0x60/.
[36510.725930][T225960] rcu: blocking rcu_node structures:
[36510.725933][T225960] 
[36510.725942][T225960] Task dump for CPU 5:
[36510.725950][T225960] task:android.hardwar state:R
[36510.725952][T225960]   running task    
[36510.725961][T225960]  stack:    0 pid: 1098 ppid:     1 flags:0x04000002
[36510.725971][T225960] Call trace:
[36510.725992][T225960]  __switch_to+0x244/0x4e4
[36510.725999][T225960] Task dump for CPU 6:
[36510.726006][T225960] task:kworker/u16:4   state:R
[36510.726008][T225960]   running task    
[36510.726015][T225960]  stack:    0 pid:14626 ppid:     2 flags:0x0000002a
[36510.726053][T225960] Workqueue: events_unbound mmstat_work_handler.cfi_jt [trace_mmstat]
[36510.726056][T225960] 
[36510.726065][T225960] Call trace:
[36510.726072][T225960]  __switch_to+0x244/0x4e4
[36510.726086][T225960]  mmstat_work_handler+0x50/0x80 [trace_mmstat]
[36510.726100][T225960]  process_one_work+0x248/0x780
[36510.726107][T225960]  worker_thread+0x438/0xbd8
[36510.726115][T225960]  kthread+0x150/0x200
[36510.726124][T225960]  ret_from_fork+0x10/0x30
[36510.729896][T200417] [DLPT] imix_r==0, skip
[36511.238110][T25910] timesync host boottime 36511196418175
[36511.382150][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1936ms> Tput: 1488(0.001mbps) [296:4:66:1][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36511.382211][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13304,0,0,8013,7994,0,7787,207,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36511.389894][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7777 us]
[36511.390062][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:4:0:0] C:[10:20:50:80]=[4:0:0:0:0] M:[5:10:20:50]=[4:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36511.390116][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23623 3249] T[2263 2263 2263 / 10950 10950 10949 10949] R[8013 / 11954] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7888] rxreg[16834]
[36511.391289][T25910] [connlog] wifi_mcu cache is full.
[36511.391533][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36511.391567][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36511.407490][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5050
[36511.409306][T726018] i2c_error_count_get 0
[36511.409331][T726018] authentic_get 1
[36511.409358][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4397 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36511.409373][T726018] charge_done_get 1
[36511.409393][T726018] capacity_raw_get 9904
[36511.409409][T726018] fastcharge_mode_set 0
[36511.409422][T726018] monitor_delay_set 30000
[36511.409441][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36511.409455][T726018] capacity_raw_get 9904
[36511.409470][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3
[36511.409482][T726018] get_index: value = 4397, index[0] = 3, index[1] = 3, new_index = 3
[36511.409491][T726018] handle_step_charge index = 3
[36511.409495][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36511.409508][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36511.409518][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36511.417264][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36511.417298][T726018] connector_temp_get 242
[36511.417313][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36511.449457][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36512.390442][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1008ms> Tput: 1104(0.001mbps) [74:1:66:1][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36512.390473][T203040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13306,0,0,8014,7995,0,7788,207,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36512.396842][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[6339 us]
[36512.396934][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:1:0:0] C:[10:20:50:80]=[1:0:0:0:0] M:[5:10:20:50]=[1:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36512.396955][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23627 3249] T[2263 2263 2263 / 10951 10951 10950 10950] R[8014 / 11956] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7889] rxreg[16837]
[36512.402079][T25910] [connlog] wifi_fw irq counter = 2821
[36512.402337][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36512.402358][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36512.429962][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5043
[36512.431853][T726018] i2c_error_count_get 0
[36512.431876][T726018] authentic_get 1
[36512.431902][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36512.431916][T726018] charge_done_get 1
[36512.431936][T726018] capacity_raw_get 9904
[36512.431951][T726018] fastcharge_mode_set 0
[36512.431963][T726018] monitor_delay_set 30000
[36512.431976][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36512.431985][T726018] capacity_raw_get 9904
[36512.431996][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36512.432006][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36512.432015][T726018] handle_step_charge index = 3
[36512.432020][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36512.432033][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36512.432044][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36512.439150][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36512.439179][T726018] connector_temp_get 242
[36512.439190][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36512.554353][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36513.454984][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36513.456922][T726018] i2c_error_count_get 0
[36513.456947][T726018] authentic_get 1
[36513.456975][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36513.456991][T726018] charge_done_get 1
[36513.457011][T726018] capacity_raw_get 9904
[36513.457027][T726018] fastcharge_mode_set 0
[36513.457040][T726018] monitor_delay_set 30000
[36513.457058][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36513.457073][T726018] capacity_raw_get 9904
[36513.457088][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36513.457098][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36513.457107][T726018] handle_step_charge index = 3
[36513.457112][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36513.457124][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36513.457134][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36513.465010][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36513.465045][T726018] connector_temp_get 242
[36513.465060][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36513.541869][C400000] [name:spm&][RC] ratio, duration_ms:10240, bus26m:0%, syspll:0%, dram:0%
[36513.541912][T423900] [ccci1/net]to:clr(0:0)

[36513.541924][T423900] [ccci1/net]ccmni0(1,1), irat_MD1, rx=(4,324,488), tx=(1,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x3,0x81,0x0,0x0)
[36513.541928][T423900] [ccci1/net]to:clr(0:0)

[36513.541933][T423900] [ccci1/net]ccmni1(0,0), irat_MD1, rx=(1028,677154,488), tx=(926,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x2,0x80,0x1,0x1)
[36513.541937][T423900] [ccci1/net]to:clr(0:0)

[36513.541942][T423900] [ccci1/net]ccmni2(0,0), irat_MD1, rx=(24,6251,488), tx=(29,0,0), txq_len=(0,0), tx_drop=(0,0,0), rx_drop=(0,0), tx_busy=(0,0), sta=(0x2,0x80,0x1,0x1)
[36513.542013][T423900] [ccci1/cif]total cnt=14262;rxq0 isr_cnt=2422;rxq1 isr_cnt=81;rxq2 isr_cnt=0;rxq3 isr_cnt=2;rxq4 isr_cnt=3468;rxq5 isr_cnt=8274;rxq6 isr_cnt=1;rxq7 isr_cnt=18;rxq8 isr_cnt=0;rxq9 isr_cnt=0;rxq10 isr_cnt=0;rxq11 isr_cnt=0;rxq12 isr_cnt=0;rxq13 isr_cnt=0;rxq14 isr_cnt=0;rxq15 isr_cnt=1;rxq16 isr_cnt=0;rxq17 isr_cnt=0;rxq18 isr_cnt=0;rxq19 isr_cnt=0;rxq20 isr_cnt=0;rxq21 isr_cnt=0;rxq22 isr_cnt=0;rxq23 isr_cnt=0;
[36513.797699][C606178] [ccci1/dpmaif]net txq0-3(status=0xf)[2048]:2048-404-404(0x0), 2048-551-551(0x0), 2048-0-0(0x0), 2048-1-1(0x0)
[36513.797709][C606178] [ccci1/dpmaif]Current txq pos: w/r/rel=(808,808,808)(1102,1102,1102)(0,0,0)(2,2,2), tx_busy=0,0,0,0
[36514.053689][C501098] [name:spm&][SPM] system_bus didn't enter MCUSYS off, MCUSYS cnt is no update
[36514.406209][T703040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <2016ms> Tput: 816(0.000mbps) [74:1:132:2][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36514.406240][T703040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13309,0,0,8016,7997,0,7790,207,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36514.413368][T403042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7074 us]
[36514.413408][T403042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:1:0:0] C:[10:20:50:80]=[1:0:0:0:0] M:[5:10:20:50]=[1:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36514.413416][T403042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23631 3251] T[2263 2263 2263 / 10952 10952 10951 10951] R[8016 / 11958] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7890] rxreg[16841]
[36514.415775][T25910] [connlog] wifi_fw irq counter = 2822
[36514.417748][T401809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36514.417984][T401809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36514.472655][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36514.477584][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5037
[36514.479549][T726018] i2c_error_count_get 0
[36514.479571][T726018] authentic_get 1
[36514.479589][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36514.479599][T726018] charge_done_get 1
[36514.479615][T726018] capacity_raw_get 9904
[36514.479626][T726018] fastcharge_mode_set 0
[36514.479635][T726018] monitor_delay_set 30000
[36514.479647][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36514.479656][T726018] capacity_raw_get 9904
[36514.479667][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36514.479678][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36514.479686][T726018] handle_step_charge index = 3
[36514.479692][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36514.479704][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36514.479714][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36514.487268][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36514.487292][T726018] connector_temp_get 242
[36514.487302][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36514.715457][  T273] input_suspend_get 0
[36514.715505][  T273] input_suspend_get 0
[36514.722353][  T273] Vbat=4398 vbats=4410 vbus:5056 ibus:120 I=0 T=25 uisoc:100 type:usb>usb pd:3 swchg_ibat:0 cv:4450000
[36514.722407][  T273] input_suspend_get 0
[36514.722437][  T273] input_suspend_get 0
[36514.722463][  T273] input_suspend_get 0
[36514.722495][  T273] mtk_charger_start_timer: alarm timer start:0, 36524 680810713
[36514.731021][T200273] input_suspend_get 0
[36514.731062][T200273] input_suspend_get 0
[36514.753578][  T273] tmp:25 (jeita:0 sm:0 cv:0 en:0) (sm:1) en:1 c:0 s:0 ov:0 sc:0 1 1 saf_cmd:-1 bat_mon:1 0
[36514.761362][T200273] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36514.761398][T200273] select_cv:cv=4450
[36514.761425][T200273] charging_current_limit(uA) 1475000 1500000 300000 300000
[36514.761448][T200273] C to C set input current 1500mA charging
[36514.761845][T200273] m:0 chg1:-1,-1,1475,1500 chg2:-1,-1,0,0 dvchg1:-1 sc:1500000 -1 0 type:4:3 usb_unlimited:0 usbif:0 usbsm:0 aicl:1475000 atm:0 bm:0 b:1 mtbf:0
[36514.761887][T200273] do_algorithm is_basic:1
[36514.762257][  T273] do_algorithm:old_cv=0,cv=4450, vbat_mon_en=1
[36514.801057][  T273] mt6375-chg 11280000.i2c:mt6375@34:chg: mt6375_dump_registers CC = 1500mA, AICR = 1475mA, MIVR = 4600mV, IEOC = 200mA, CV = 4450mV
[36514.801057][  T273] VBUS = 5050mV, IBUS = 187mA, VBAT = 4410mV, IBAT = 0mA, VSYS = 4456mV
[36514.801057][  T273] CHG_STAT = 0x01, CHG_STAT0 = 0xC1, CHG_STAT1 = 0x00, CHG_TOP1 = 0xB2, CHG_TOP2 = 0x43, CHG_EOC = 0x30
[36514.803568][ T1189] [smartcharging] [sc1]en:0 t:0,80000,46901,33099 t:3600,29499,-1,-1 c:0,2000 ibus:0 uisoc:100,80 s:3000 ans:ignore
[36514.803835][  T273] input_suspend_get 0
[36514.810634][  T273] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5050
[36514.822203][T726018] power_debug_work: start 
[36514.822337][T726018] active wake lock : 11201000.usb0,last_time:8721
[36515.341737][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7337 us]
[36515.400512][T203042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36515.502983][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5062
[36515.504631][T726018] i2c_error_count_get 0
[36515.504655][T726018] authentic_get 1
[36515.504682][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36515.504698][T726018] charge_done_get 1
[36515.504719][T726018] capacity_raw_get 9904
[36515.504735][T726018] fastcharge_mode_set 0
[36515.504748][T726018] monitor_delay_set 30000
[36515.504767][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36515.504781][T726018] capacity_raw_get 9904
[36515.504798][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36515.504814][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36515.504827][T726018] handle_step_charge index = 3
[36515.504834][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36515.504856][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36515.504871][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36515.512594][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36515.512625][T726018] connector_temp_get 242
[36515.512635][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36515.590745][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1184ms> Tput: 2216(0.002mbps) [262:3:66:1][0:0:0:0][0:0:0:0][0:0:0:0] Pending:2/4096 [0:2:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36515.590803][T303040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13312,0,0,8017,7998,0,7791,207,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36515.598358][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7711 us]
[36515.598666][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:3:0:0] C:[10:20:50:80]=[2:0:0:0:0] M:[5:10:20:50]=[2:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36515.598722][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23639 3253] T[2263 2263 2263 / 10955 10955 10953 10953] R[8017 / 11962] T_R[2 0 0 0 0]  R_R[0 0 0 0] Tok[2/4096] Rfb[4150/4150] txreg[7892] rxreg[16846]
[36515.601086][T301809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36515.601119][T301809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36515.605909][T303040] [wlan][wlan][3040]cnmTimerStartTimer:(CNM INFO) [WLAN-LP] Start timer 000000005bb70dbc 200 ms -handler(qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895])
[36515.658704][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36515.811233][ T3040] [wlan][3040]cnmTimerDoTimeOutCheck:(CNM INFO) timer timeout, timer 000000005bb70dbc func qmHandleReorderBubbleTimeout.cfi_jt [wlan_drv_gen4m_6895]
[36515.854405][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7744 us]
[36515.912629][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36516.527081][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5050
[36516.529046][T726018] i2c_error_count_get 0
[36516.529071][T726018] authentic_get 1
[36516.529098][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36516.529113][T726018] charge_done_get 1
[36516.529134][T726018] capacity_raw_get 9904
[36516.529149][T726018] fastcharge_mode_set 0
[36516.529162][T726018] monitor_delay_set 30000
[36516.529181][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36516.529195][T726018] capacity_raw_get 9904
[36516.529209][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36516.529219][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36516.529228][T726018] handle_step_charge index = 3
[36516.529234][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36516.529246][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36516.529256][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36516.537030][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36516.537059][T726018] connector_temp_get 242
[36516.537071][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36516.870425][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1280ms> Tput: 1456(0.001mbps) [168:2:66:1][0:0:0:0][0:0:0:0][0:0:0:0] Pending:1/4096 [0:1:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36516.870454][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13315,0,0,8018,7999,0,7791,208,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36516.878255][T203042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7755 us]
[36516.878342][T203042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:2:0:0] C:[10:20:50:80]=[3:0:0:0:0] M:[5:10:20:50]=[3:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36516.878363][T203042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23646 3256] T[2263 2263 2263 / 10957 10957 10956 10956] R[8018 / 11966] T_R[1 0 0 0 0]  R_R[0 0 0 0] Tok[1/4096] Rfb[4150/4150] txreg[7894] rxreg[16851]
[36516.879852][T25910] [connlog] wifi_fw irq counter = 2824
[36516.880118][T701809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36516.880142][T701809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36516.936711][ T3042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36517.550130][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: get mt6375 charger status=4 vbus = 5056
[36517.552326][T726018] i2c_error_count_get 0
[36517.552349][T726018] authentic_get 1
[36517.552374][T726018] [CHARGE_LOOP] TYPE = [3 0 12 0], BMS = [100 4398 0 256], FULL = [0 1 1 0 0], thermal_level=0, FFC = 0, sw_cv=0, gauge_authentic=1
[36517.552388][T726018] charge_done_get 1
[36517.552408][T726018] capacity_raw_get 9904
[36517.552424][T726018] fastcharge_mode_set 0
[36517.552436][T726018] monitor_delay_set 30000
[36517.552453][T726018] check_full_recharge diff_fv_val = 0, iterm = 200, iterm_effective = 200, fv_effective = 4450, full_count = 0, recharge_count = 0
[36517.552466][T726018] capacity_raw_get 9904
[36517.552481][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3
[36517.552495][T726018] get_index: value = 4398, index[0] = 3, index[1] = 3, new_index = 3
[36517.552504][T726018] handle_step_charge index = 3
[36517.552509][T726018] get_index: value = 256, index[0] = 4, index[1] = 4
[36517.552522][T726018] get_index: value = 256, index[0] = 4, index[1] = 4, new_index = 4
[36517.552532][T726018] handle_jeita_charge index = 4,jeita_chg_fcc = 12400
[36517.558628][T726018] mt6375-chg 11280000.i2c:mt6375@34:chg: read ADC_CHANNEL_TS value = 242
[36517.558659][T726018] connector_temp_get 242
[36517.558670][T726018] monitor_typec_burn get typec temp=242 otg_enable=0
[36517.895184][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) <1025ms> Tput: 1152(0.001mbps) [148:2:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Pending:2/4096 [0:2:0:0][0:0:0:0][0:0:0:0][0:0:0:0] Used:0/4096/1024 [0][0][0][0][0] LQ[9615:8591:2758] lv:0 th:5 fg:0x7 TxDp[ST:BS:FO:QM:DP]:0:0:0:0:295
[36517.895244][ T3040] [wlan][3040]kalPerMonUpdate:(SW4 INFO) ndevdrp:[0:0:0:0][0:0:0:0][0:0:0:0][0:0:0:0] drv[RM,IL,SL,RI,RT,RM,RW,RA,RB,DT,NS,IB,HS,LS,DD,ME,BD,NI,DR,TE,CE,DN,FE,DE,IE,TME,SC,SI]:13316,0,0,8018,7999,0,7791,208,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.
[36517.902839][ T3042] [wlan][3042]halSetDriverOwn:(INIT INFO) DRIVER OWN Done[7602 us]
[36517.903143][ T3042] [wlan][3042]kalDumpMsduReportStats:(HAL INFO) TX_Delay D:[1:5:10:20]=[0:0:2:0:0] C:[10:20:50:80]=[1:0:0:0:0] M:[5:10:20:50]=[1:0:0:0:0] F:[10:20:50:80]=[0:0:0:0:0] Txfail:0
[36517.903202][ T3042] [wlan][3042]kalDumpHifStats:(HAL INFO) I[23649 3256] T[2263 2263 2263 / 10959 10959 10957 10957] R[8018 / 11968] T_R[2 0 0 0 0]  R_R[0 0 0 0] Tok[2/4096] Rfb[4150/4150] txreg[7895] rxreg[16853]
[36517.907368][T201809] [WIFI-FW] fw_log_wifi_read[L]: fw_log_wifi_read len --> 32768
[36517.907403][T201809] [WIFI-FW] fw_log_wifi_read[L]: WIFI_FW_LOG_IOCTL_ON_OFF result=1, last value=1
[36517.957744][T303042] [wlan][3042]halSetFWOwn:(INIT INFO) FW OWN:1, IntSta:0x00000010
[36518.149850][C400000] watchdogd on CPU 0
[36518.149874][C400000] task:watchdogd       state:S
[36518.149878][C400000]  stack:    0 pid:  121 ppid:     2 flags:0x00000008
[36518.149885][C400000] Call trace:
[36518.149901][C400000]  __switch_to+0x244/0x4e4
[36518.149917][C400000]  __schedule+0x5b4/0xbe8
[36518.149921][C400000]  schedule+0x80/0x160
[36518.149933][C400000]  kthread_worker_fn+0x90/0x58c
[36518.149945][C400000]  kthread+0x150/0x200
[36518.149956][C400000]  ret_from_fork+0x10/0x30
[36518.152087][C400000] task on CPU0
[36518.152100][C400000] task:swapper/0       state:R
[36518.152101][C400000]   running task    
[36518.152104][C400000]  stack:    0 pid:    0 ppid:     0 flags:0x00000008
[36518.152109][C400000] Call trace:
[36518.152112][C400000]  __switch_to+0x244/0x4e4
[36518.152118][C400000]  0x3688ecdfe5
[36518.152121][C400000] task on CPU1
[36518.152124][C400000] task:migration/1     state:R
[36518.152125][C400000]   running task    
[36518.152128][C400000]  stack:    0 pid:   19 ppid:     2 flags:0x0000000a
[36518.152132][C400000] Call trace:
[36518.152134][C400000]  __switch_to+0x244/0x4e4
[36518.152136][C400000]  0x0
[36518.152145][C400000]  cpu_stopper_thread+0x138/0x41c
[36518.152151][C400000]  smpboot_thread_fn+0x180/0x594
[36518.152154][C400000]  kthread+0x150/0x200
[36518.152157][C400000]  ret_from_fork+0x10/0x30
[36518.152159][C400000] task on CPU2
[36518.152161][C400000] task:.unixsocketdemo state:R
[36518.152161][C400000]   running task    
[36518.152164][C400000]  stack:    0 pid:23673 ppid:  1055 flags:0x04000800
[36518.152168][C400000] Call trace:
[36518.152170][C400000]  __switch_to+0x244/0x4e4
[36518.152173][C400000] task on CPU3
[36518.152176][C400000] task:swapper/3       state:R
[36518.152177][C400000]   running task    
[36518.152179][C400000]  stack:    0 pid:    0 ppid:     1 flags:0x00000008
[36518.152183][C400000] Call trace:
[36518.152185][C400000]  __switch_to+0x244/0x4e4
[36518.152187][C400000]  0x3688284fe2
[36518.152190][C400000] task on CPU4
[36518.152192][C400000] task:swapper/4       state:R
[36518.152193][C400000]   running task    
[36518.152195][C400000]  stack:    0 pid:    0 ppid:     1 flags:0x00000008
[36518.152199][C400000] Call trace:
[36518.152207][C400000]  dump_backtrace.cfi_jt+0x0/0x8
[36518.152213][C400000]  sched_show_task+0x198/0x214
[36518.152226][C400000]  kwdt_dump_func+0xfc/0x1fc [aee_hangdet]
[36518.152236][C400000]  aee_dump_timer_func+0x18c/0x204 [aee_hangdet]
[36518.152247][C400000]  call_timer_fn+0x58/0x314
[36518.152252][C400000]  expire_timers+0xe0/0x438
[36518.152255][C400000]  __run_timers+0x1f0/0x328
[36518.152257][C400000]  run_timer_softirq+0x28/0x58
[36518.152260][C400000]  efi_header_end+0x168/0x5ec
[36518.152266][C400000]  __irq_exit_rcu+0x108/0x124
[36518.152278][C400000]  __handle_domain_irq+0x118/0x1e4
[36518.152288][C400000]  gic_handle_irq.30283+0x6c/0x2b8
[36518.152292][C400000]  el1_irq+0xe4/0x1c0
[36518.152306][C400000]  cpuidle_enter_state+0x3a4/0xa04
[36518.152316][C400000]  do_idle+0x308/0x574
[36518.152322][C400000]  cpu_startup_entry+0x84/0x90
[36518.152333][C400000]  secondary_start_kernel+0x204/0x27c
[36518.152339][C400000] task on CPU5
[36518.152345][C400000] task:android.hardwar state:R
[36518.152351][C400000]   running task    
[36518.152356][C400000]  stack:    0 pid: 1098 ppid:     1 flags:0x04000002
[36518.152366][C400000] Call trace:
[36518.152373][C400000]  __switch_to+0x244/0x4e4
[36518.152378][C400000] task on CPU6
[36518.152386][C400000] task:kworker/u16:4   state:R
[36518.152389][C400000]   running task    
[36518.152396][C400000]  stack:    0 pid:14626 ppid:     2 flags:0x0000002a
[36518.152418][C400000] Workqueue: events_unbound mmstat_work_handler.cfi_jt [trace_mmstat]
[36518.152423][C400000] 
[36518.152430][C400000] Call trace:
[36518.152432][C400000]  __switch_to+0x244/0x4e4
[36518.152443][C400000]  mmstat_work_handler+0x50/0x80 [trace_mmstat]
[36518.152453][C400000]  process_one_work+0x248/0x780
[36518.152459][C400000]  worker_thread+0x438/0xbd8
[36518.152466][C400000]  kthread+0x150/0x200
[36518.152471][C400000]  ret_from_fork+0x10/0x30
[36518.152479][C400000] task on CPU7
[36518.152485][C400000] task:swapper/7       state:R
[36518.152485][C400000]   running task    
[36518.152491][C400000]  stack:    0 pid:    0 ppid:     1 flags:0x00000008
[36518.152499][C400000] Call trace:
[36518.152506][C400000]  __switch_to+0x244/0x4e4
[36518.152514][C400000]  0x3688e1192b
[36518.152522][C400000] kick=0x9d,check=0xff
[36518.247046][C400000] Kernel Offset: 0x2249c00000 from 0xffffffc010000000
[36518.247053][C400000] PHYS_OFFSET: 0x40000000
