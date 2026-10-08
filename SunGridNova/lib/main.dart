import 'dart:async';
import 'dart:math' as math;
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'services/aws_iot_service.dart';
import 'services/ble_provisioning_service.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const SunGridNovaApp());
}

/// SunGridNova: Executive-Grade Industrial Solar IoT Application
class SunGridNovaApp extends StatelessWidget {
  const SunGridNovaApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'SunGridNova',
      debugShowCheckedModeBanner: false,
      themeMode: ThemeMode.dark,
      darkTheme: ThemeData(
        brightness: Brightness.dark,
        scaffoldBackgroundColor: const Color(0xFF090D16),
        primaryColor: const Color(0xFFF59E0B),
        cardColor: const Color(0xFF111827),
        fontFamily: 'SF Pro Display',
        colorScheme: const ColorScheme.dark(
          primary: Color(0xFFF59E0B),
          secondary: Color(0xFF38BDF8),
          surface: Color(0xFF111827),
          error: Color(0xFFEF4444),
        ),
      ),
      home: const MainNavigationShell(),
    );
  }
}

// ---------------------------------------------------------------------------
// TELEMETRY STATE MODEL
// ---------------------------------------------------------------------------
class InverterTelemetry {
  String thingId;
  bool isLiveAws;
  AwsConnectionStatus awsStatus;
  DateTime? lastPacketTime;
  double solarVolt;
  double solarWatts;
  double battVolt;
  double battCurrent;
  int battPercent;
  double gridVolt;
  double gridFreq;
  double acOutVolt;
  double loadWatts;
  int loadPercent;
  double heatSinkTemp;
  double dcBoostVolt;
  double todayKwh;
  double lifetimeMwh;
  String feedSource;
  String opMode;
  int errorCode;
  bool isOnline;
  String connectedSsid;
  String ipAddress;
  int wifiRssi;
  String vendorName;
  String modelName;
  String serialNumber;
  String hardwareVersion;
  String vendorContact;
  String vendorWebsite;

  InverterTelemetry({
    this.thingId = 'SunGridNova-49F8',
    this.isLiveAws = false,
    this.awsStatus = AwsConnectionStatus.disconnected,
    this.lastPacketTime,
    this.solarVolt = 78.4,
    this.solarWatts = 2450.0,
    this.battVolt = 26.8,
    this.battCurrent = 16.4,
    this.battPercent = 82,
    this.gridVolt = 228.0,
    this.gridFreq = 50.0,
    this.acOutVolt = 230.0,
    this.loadWatts = 840.0,
    this.loadPercent = 42,
    this.heatSinkTemp = 38.5,
    this.dcBoostVolt = 385.0,
    this.todayKwh = 14.8,
    this.lifetimeMwh = 3.84,
    this.feedSource = 'SOLAR PV',
    this.opMode = 'SMART HYBRID',
    this.errorCode = 0,
    this.isOnline = true,
    this.connectedSsid = '',
    this.ipAddress = '',
    this.wifiRssi = -100,
    this.vendorName = '',
    this.modelName = '',
    this.serialNumber = '',
    this.hardwareVersion = 'HW-V2.1',
    this.vendorContact = '',
    this.vendorWebsite = '',
  });
}

// ---------------------------------------------------------------------------
// MAIN NAVIGATION SHELL
// ---------------------------------------------------------------------------
class MainNavigationShell extends StatefulWidget {
  const MainNavigationShell({super.key});

  @override
  State<MainNavigationShell> createState() => _MainNavigationShellState();
}

class _MainNavigationShellState extends State<MainNavigationShell> {
  int _currentIndex = 0;
  final InverterTelemetry _telemetry = InverterTelemetry();
  final _awsService = AwsIotService();
  final _bleService = BleProvisioningService();
  StreamSubscription? _awsStatusSub;
  StreamSubscription? _awsTelemetrySub;
  StreamSubscription? _awsAlertSub;
  StreamSubscription? _bleSub;
  Timer? _simTimer;
  double _simPhase = 0.0;

  @override
  void initState() {
    super.initState();

    // Connect to AWS IoT Core via TLS 1.2 mTLS
    _awsService.connect();

    _awsStatusSub = _awsService.statusStream.listen((status) {
      if (!mounted) return;
      setState(() {
        _telemetry.awsStatus = status;
      });
    });

    _bleSub = _bleService.isProvisionedStream.listen((_) {
      if (!mounted) return;
      setState(() {
        if (_bleService.vendorName != null && _bleService.vendorName!.isNotEmpty) {
          _telemetry.vendorName = _bleService.vendorName!;
        }
        if (_bleService.modelName != null && _bleService.modelName!.isNotEmpty) {
          _telemetry.modelName = _bleService.modelName!;
        }
        if (_bleService.serialNumber != null && _bleService.serialNumber!.isNotEmpty) {
          _telemetry.serialNumber = _bleService.serialNumber!;
        }
        if (_bleService.hardwareVersion != null && _bleService.hardwareVersion!.isNotEmpty) {
          _telemetry.hardwareVersion = _bleService.hardwareVersion!;
        }
        if (_bleService.vendorContact != null && _bleService.vendorContact!.isNotEmpty) {
          _telemetry.vendorContact = _bleService.vendorContact!;
        }
        if (_bleService.vendorWebsite != null && _bleService.vendorWebsite!.isNotEmpty) {
          _telemetry.vendorWebsite = _bleService.vendorWebsite!;
        }
        if (_bleService.inverterSsid != null && _bleService.inverterSsid!.isNotEmpty) {
          _telemetry.connectedSsid = _bleService.inverterSsid!;
        }
        if (_bleService.inverterIp != null && _bleService.inverterIp!.isNotEmpty) {
          _telemetry.ipAddress = _bleService.inverterIp!;
        }
      });
    });

    _awsTelemetrySub = _awsService.telemetryStream.listen((data) {
      if (!mounted) return;
      setState(() {
        _telemetry.isLiveAws = true;
        _telemetry.lastPacketTime = DateTime.now();
        if (data['thing'] != null) _telemetry.thingId = data['thing'].toString();
        if (data['sol_v'] != null) _telemetry.solarVolt = (data['sol_v'] as num).toDouble();
        if (data['bat_v'] != null) {
          _telemetry.battVolt = (data['bat_v'] as num).toDouble();
          _telemetry.battPercent = ((_telemetry.battVolt - 21.0) / (27.6 - 21.0) * 100).clamp(0, 100).round();
        }
        if (data['grid_v'] != null) _telemetry.gridVolt = (data['grid_v'] as num).toDouble();
        if (data['ac_out'] != null) _telemetry.acOutVolt = (data['ac_out'] as num).toDouble();
        if (data['load_pct'] != null) {
          _telemetry.loadPercent = (data['load_pct'] as num).toInt();
          _telemetry.loadWatts = _telemetry.loadPercent * 20.0;
        }
        if (data['heat'] != null) _telemetry.heatSinkTemp = (data['heat'] as num).toDouble();
        if (data['dc_b'] != null) _telemetry.dcBoostVolt = (data['dc_b'] as num).toDouble();
        if (data['err'] != null) _telemetry.errorCode = (data['err'] as num).toInt();
        if (data['chg_a'] != null) {
          final chg = (data['chg_a'] as num).toDouble();
          final dis = (data['dis_a'] as num?)?.toDouble() ?? 0.0;
          _telemetry.battCurrent = chg > 0 ? chg : -dis;
          if (_telemetry.solarVolt > 15) {
            _telemetry.solarWatts = _telemetry.solarVolt * (chg + (_telemetry.loadWatts / math.max(_telemetry.solarVolt, 1.0)));
          } else {
            _telemetry.solarWatts = 0.0;
          }
        }
        if (data['ssid'] != null && data['ssid'].toString().isNotEmpty) {
          _telemetry.connectedSsid = data['ssid'].toString();
        }
        if (data['ip'] != null && data['ip'].toString().isNotEmpty) {
          _telemetry.ipAddress = data['ip'].toString();
        }
        if (data['vendor'] != null && data['vendor'].toString().trim().isNotEmpty) {
          _telemetry.vendorName = data['vendor'].toString().trim();
        } else if (data['oem'] != null && data['oem'].toString().trim().isNotEmpty) {
          _telemetry.vendorName = data['oem'].toString().trim();
        }
        if (data['model'] != null && data['model'].toString().trim().isNotEmpty) {
          _telemetry.modelName = data['model'].toString().trim();
        }
        if (data['sn'] != null && data['sn'].toString().trim().isNotEmpty) {
          _telemetry.serialNumber = data['sn'].toString().trim();
        }
        if (data['hw'] != null && data['hw'].toString().trim().isNotEmpty) {
          _telemetry.hardwareVersion = data['hw'].toString().trim();
        }
        if (data['contact'] != null && data['contact'].toString().trim().isNotEmpty) {
          _telemetry.vendorContact = data['contact'].toString().trim();
        }
        if (data['site'] != null && data['site'].toString().trim().isNotEmpty) {
          _telemetry.vendorWebsite = data['site'].toString().trim();
        }
      });
    });

    _awsAlertSub = _awsService.alertStream.listen((data) {
      if (!mounted) return;
      if (data['code'] != null) {
        setState(() {
          _telemetry.errorCode = (data['code'] as num).toInt();
        });
      }
    });

    // 1-Second real-time ticker
    _simTimer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (!mounted) return;
      final bool isLiveStale = _telemetry.lastPacketTime == null ||
          DateTime.now().difference(_telemetry.lastPacketTime!).inSeconds > 30;

      setState(() {
        _simPhase += 0.08;
        if (isLiveStale) {
          _telemetry.isLiveAws = false;
          // Only generate simulated wave values if we haven't received ANY real packet yet!
          if (_telemetry.lastPacketTime == null) {
            _telemetry.solarVolt = 76.0 + 3.0 * math.sin(_simPhase);
            _telemetry.solarWatts = 2400.0 + 150.0 * math.sin(_simPhase * 1.2);
            _telemetry.battVolt = 26.7 + 0.2 * math.cos(_simPhase * 0.5);
            _telemetry.loadWatts = 820.0 + 80.0 * math.sin(_simPhase * 1.5);
            _telemetry.loadPercent = (_telemetry.loadWatts / 20.0).round();
            _telemetry.heatSinkTemp = 38.0 + 1.2 * math.sin(_simPhase * 0.3);
          }
        }
      });
    });
  }

  @override
  void dispose() {
    _simTimer?.cancel();
    _awsStatusSub?.cancel();
    _awsTelemetrySub?.cancel();
    _awsAlertSub?.cancel();
    _bleSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final screens = [
      DashboardScreen(telemetry: _telemetry),
      DailyProductionScreen(telemetry: _telemetry),
      ProvisioningScreen(telemetry: _telemetry),
      VendorScreen(telemetry: _telemetry),
    ];

    return Scaffold(
      body: screens[_currentIndex],
      bottomNavigationBar: Container(
        decoration: BoxDecoration(
          color: const Color(0xFF0F172A).withValues(alpha: 0.95),
          border: const Border(
            top: BorderSide(color: Color(0xFF1E293B), width: 1.0),
          ),
        ),
        child: BottomNavigationBar(
          currentIndex: _currentIndex,
          onTap: (idx) => setState(() => _currentIndex = idx),
          backgroundColor: Colors.transparent,
          elevation: 0,
          type: BottomNavigationBarType.fixed,
          selectedItemColor: const Color(0xFFF59E0B),
          unselectedItemColor: const Color(0xFF64748B),
          selectedLabelStyle: const TextStyle(fontWeight: FontWeight.w600, fontSize: 12),
          unselectedLabelStyle: const TextStyle(fontSize: 11),
          items: const [
            BottomNavigationBarItem(
              icon: Icon(CupertinoIcons.bolt_circle_fill),
              label: 'Power Flow',
            ),
            BottomNavigationBarItem(
              icon: Icon(CupertinoIcons.chart_bar_alt_fill),
              label: 'Production',
            ),
            BottomNavigationBarItem(
              icon: Icon(CupertinoIcons.wifi),
              label: 'Wi-Fi Setup',
            ),
            BottomNavigationBarItem(
              icon: Icon(CupertinoIcons.building_2_fill),
              label: 'Vendor',
            ),
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// SCREEN 1: EXECUTIVE LIVE POWER FLOW DASHBOARD
// ---------------------------------------------------------------------------
class DashboardScreen extends StatefulWidget {
  final InverterTelemetry telemetry;
  const DashboardScreen({super.key, required this.telemetry});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> with SingleTickerProviderStateMixin {
  late AnimationController _flowController;

  @override
  void initState() {
    super.initState();
    _flowController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 2),
    )..repeat();
  }

  @override
  void dispose() {
    _flowController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final t = widget.telemetry;

    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 14.0, vertical: 8.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // 1. Top Brand & Model Header
            Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(4),
                  decoration: BoxDecoration(
                    color: const Color(0xFF1E293B),
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(color: const Color(0xFF334155)),
                    boxShadow: [
                      BoxShadow(
                        color: const Color(0xFFF59E0B).withValues(alpha: 0.25),
                        blurRadius: 8,
                        offset: const Offset(0, 2),
                      )
                    ],
                  ),
                  child: Image.asset(
                    'assets/logo.png',
                    width: 28,
                    height: 28,
                    fit: BoxFit.contain,
                    errorBuilder: (ctx, err, stack) => const Icon(
                      CupertinoIcons.sun_max_fill,
                      color: Color(0xFFF59E0B),
                      size: 24,
                    ),
                  ),
                ),
                const SizedBox(width: 10),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        t.vendorName.isNotEmpty ? t.vendorName : 'SunGridNova',
                        style: const TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.w800,
                          letterSpacing: 0.4,
                          color: Colors.white,
                        ),
                        overflow: TextOverflow.ellipsis,
                      ),
                      Text(
                        '${t.modelName.isNotEmpty ? t.modelName : "HYBRID MPPT"} • ${t.thingId}',
                        style: TextStyle(
                          fontSize: 10,
                          fontWeight: FontWeight.w600,
                          color: Colors.white.withValues(alpha: 0.5),
                        ),
                        overflow: TextOverflow.ellipsis,
                      ),
                    ],
                  ),
                ),
                GestureDetector(
                  onTap: () {
                    if (t.awsStatus != AwsConnectionStatus.connected) {
                      AwsIotService().connect();
                    }
                  },
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
                    decoration: BoxDecoration(
                      color: t.isLiveAws
                          ? const Color(0xFF064E3B).withValues(alpha: 0.8)
                          : (t.awsStatus == AwsConnectionStatus.connecting
                              ? const Color(0xFF78350F).withValues(alpha: 0.8)
                              : const Color(0xFF1E293B)),
                      borderRadius: BorderRadius.circular(20),
                      border: Border.all(
                        color: t.isLiveAws
                            ? const Color(0xFF10B981)
                            : (t.awsStatus == AwsConnectionStatus.connecting
                                ? const Color(0xFFF59E0B)
                                : const Color(0xFF64748B)),
                        width: 1.2,
                      ),
                    ),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Container(
                          width: 8,
                          height: 8,
                          decoration: BoxDecoration(
                            shape: BoxShape.circle,
                            color: t.isLiveAws
                                ? const Color(0xFF10B981)
                                : (t.awsStatus == AwsConnectionStatus.connecting
                                    ? const Color(0xFFF59E0B)
                                    : const Color(0xFF94A3B8)),
                          ),
                        ),
                        const SizedBox(width: 6),
                        Text(
                          t.isLiveAws
                              ? 'LIVE'
                              : (t.awsStatus == AwsConnectionStatus.connecting
                                  ? 'CONNECTING'
                                  : 'OFFLINE'),
                          style: TextStyle(
                            fontSize: 10,
                            fontWeight: FontWeight.w700,
                            letterSpacing: 0.5,
                            color: t.isLiveAws
                                ? const Color(0xFF34D399)
                                : (t.awsStatus == AwsConnectionStatus.connecting
                                    ? const Color(0xFFFCD34D)
                                    : const Color(0xFF94A3B8)),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),

            const SizedBox(height: 10),

            // 2. Fault Protection Alert Banner (Only visible on trip)
            if (t.errorCode != 0)
              Container(
                margin: const EdgeInsets.only(bottom: 10),
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: const Color(0xFF450A0A),
                  borderRadius: BorderRadius.circular(14),
                  border: Border.all(color: const Color(0xFFEF4444), width: 1.5),
                ),
                child: Row(
                  children: [
                    const Icon(CupertinoIcons.exclamationmark_triangle_fill, color: Color(0xFFEF4444), size: 24),
                    const SizedBox(width: 10),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Text(
                            'SAFETY PROTECTION TRIP ACTIVE',
                            style: TextStyle(fontSize: 12, fontWeight: FontWeight.w800, color: Color(0xFFFCA5A5)),
                          ),
                          Text(
                            'Fault Code: ${t.errorCode} • Output Disconnected for Safety',
                            style: TextStyle(fontSize: 10.5, color: Colors.white.withValues(alpha: 0.8)),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),

            // 3. Central Interactive Power Synoptic Hub (Balanced, Prominent, Big Cards)
            Expanded(
              child: Container(
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 12),
                decoration: BoxDecoration(
                  gradient: const LinearGradient(
                    colors: [Color(0xFF0F172A), Color(0xFF1E293B)],
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                  ),
                  borderRadius: BorderRadius.circular(24),
                  border: Border.all(color: const Color(0xFF334155), width: 1.2),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.black.withValues(alpha: 0.4),
                      blurRadius: 20,
                      offset: const Offset(0, 8),
                    )
                  ],
                ),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    Row(
                      children: [
                        Text(
                          'REAL-TIME ENERGY FLOW',
                          style: TextStyle(
                            fontSize: 11,
                            fontWeight: FontWeight.w800,
                            letterSpacing: 1.2,
                            color: Colors.white.withValues(alpha: 0.6),
                          ),
                        ),
                      ],
                    ),

                    // Perfectly Aligned Symmetrical 5-Card Synoptic Flow
                    // NODE 1: Centered Solar PV Generation Node (BIG)
                    Center(
                      child: _buildSynopticNode(
                        icon: CupertinoIcons.sun_max_fill,
                        title: 'SOLAR PV ARRAY',
                        value: '${t.solarWatts.round()} W',
                        subtitle: '${t.solarVolt.toStringAsFixed(1)} V DC',
                        color: const Color(0xFFF59E0B),
                        width: 220,
                        isLarge: true,
                      ),
                    ),

                    // Vertical Flow Guide (Solar to Inverter)
                    Icon(
                      CupertinoIcons.chevron_compact_down,
                      color: const Color(0xFFF59E0B).withValues(alpha: 0.6),
                      size: 20,
                    ),

                    // ROW 2: AC Grid (Left) -- Animated Inverter Core (Center) -- AC Load (Right)
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Expanded(
                          child: _buildSynopticNode(
                            icon: CupertinoIcons.waveform_path_ecg,
                            title: 'AC GRID',
                            value: '${t.gridVolt.round()} V',
                            subtitle: '${t.gridFreq} Hz',
                            color: const Color(0xFF38BDF8),
                          ),
                        ),
                        const SizedBox(width: 10),
                        AnimatedBuilder(
                          animation: _flowController,
                          builder: (context, child) {
                            return Container(
                              width: 96,
                              height: 96,
                              decoration: BoxDecoration(
                                shape: BoxShape.circle,
                                gradient: RadialGradient(
                                  colors: [
                                    const Color(0xFFF59E0B).withValues(alpha: 0.18),
                                    Colors.transparent,
                                  ],
                                ),
                                border: Border.all(
                                  color: const Color(0xFFF59E0B).withValues(
                                    alpha: 0.5 + 0.3 * math.sin(_flowController.value * 2 * math.pi),
                                  ),
                                  width: 2.5,
                                ),
                                boxShadow: [
                                  BoxShadow(
                                    color: const Color(0xFFF59E0B).withValues(alpha: 0.25),
                                    blurRadius: 12,
                                    spreadRadius: 1,
                                  ),
                                ],
                              ),
                              child: Column(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  const Icon(CupertinoIcons.rays, color: Color(0xFFF59E0B), size: 26),
                                  const SizedBox(height: 3),
                                  const Text(
                                    'INVERTER',
                                    style: TextStyle(fontSize: 10, fontWeight: FontWeight.w900, color: Colors.white, letterSpacing: 0.5),
                                  ),
                                  Text(
                                    '${t.acOutVolt.round()} VAC ${t.gridFreq.round()}Hz',
                                    style: const TextStyle(fontSize: 8.5, fontWeight: FontWeight.bold, color: Color(0xFFFDE68A)),
                                  ),
                                ],
                              ),
                            );
                          },
                        ),
                        const SizedBox(width: 10),
                        Expanded(
                          child: _buildSynopticNode(
                            icon: CupertinoIcons.house_alt_fill,
                            title: 'AC LOAD',
                            value: '${t.loadWatts.round()} W',
                            subtitle: '${t.loadPercent}% Load',
                            color: const Color(0xFFA855F7),
                          ),
                        ),
                      ],
                    ),

                    // Vertical Flow Guide (Inverter to Battery)
                    Icon(
                      CupertinoIcons.chevron_compact_down,
                      color: const Color(0xFF10B981).withValues(alpha: 0.6),
                      size: 20,
                    ),

                    // NODE 3: Centered Battery Storage Node (BIG)
                    Center(
                      child: _buildSynopticNode(
                        icon: CupertinoIcons.battery_25,
                        title: 'BATTERY STORAGE',
                        value: '${t.battPercent}%',
                        subtitle: '${t.battVolt.toStringAsFixed(1)}V (${t.battCurrent >= 0 ? "+" : ""}${t.battCurrent.toStringAsFixed(1)}A)',
                        color: const Color(0xFF10B981),
                        width: 220,
                        isLarge: true,
                      ),
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 10),

            // 4. Executive Metric Cards (Compact 4-column strip, Zero scroll needed)
            Row(
              children: [
                Expanded(
                  child: _buildCompactMetric(
                    title: "HARVEST",
                    value: "${t.todayKwh} kWh",
                    icon: CupertinoIcons.leaf_arrow_circlepath,
                    accentColor: const Color(0xFF10B981),
                  ),
                ),
                const SizedBox(width: 6),
                Expanded(
                  child: _buildCompactMetric(
                    title: "LIFETIME",
                    value: "${t.lifetimeMwh} MWh",
                    icon: CupertinoIcons.globe,
                    accentColor: const Color(0xFF38BDF8),
                  ),
                ),
                const SizedBox(width: 6),
                Expanded(
                  child: _buildCompactMetric(
                    title: "DC BUS",
                    value: "${t.dcBoostVolt.round()} V",
                    icon: CupertinoIcons.speedometer,
                    accentColor: const Color(0xFFF59E0B),
                  ),
                ),
                const SizedBox(width: 6),
                Expanded(
                  child: _buildCompactMetric(
                    title: "TEMP",
                    value: "${t.heatSinkTemp.toStringAsFixed(1)}°C",
                    icon: CupertinoIcons.thermometer,
                    accentColor: const Color(0xFF10B981),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSynopticNode({
    required IconData icon,
    required String title,
    required String value,
    required String subtitle,
    required Color color,
    double? width,
    bool isLarge = false,
  }) {
    return Container(
      width: width,
      padding: EdgeInsets.symmetric(
        horizontal: isLarge ? 16 : 10,
        vertical: isLarge ? 10 : 8,
      ),
      decoration: BoxDecoration(
        color: const Color(0xFF0B132B),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: color.withValues(alpha: 0.7), width: 1.8),
        boxShadow: [
          BoxShadow(
            color: color.withValues(alpha: 0.25),
            blurRadius: 10,
            offset: const Offset(0, 3),
          ),
        ],
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Row(
            mainAxisSize: MainAxisSize.min,
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(icon, color: color, size: isLarge ? 22 : 18),
              const SizedBox(width: 6),
              Text(
                title,
                style: TextStyle(
                  fontSize: isLarge ? 10.5 : 9.5,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 0.6,
                  color: Colors.white.withValues(alpha: 0.75),
                ),
              ),
            ],
          ),
          const SizedBox(height: 4),
          Text(
            value,
            style: TextStyle(
              fontSize: isLarge ? 20 : 16,
              fontWeight: FontWeight.w900,
              color: Colors.white,
              letterSpacing: 0.5,
            ),
          ),
          const SizedBox(height: 2),
          Text(
            subtitle,
            style: TextStyle(
              fontSize: isLarge ? 11 : 9.5,
              fontWeight: FontWeight.w600,
              color: color,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildCompactMetric({
    required String title,
    required String value,
    required IconData icon,
    required Color accentColor,
  }) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 8),
      decoration: BoxDecoration(
        color: const Color(0xFF111827),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(color: const Color(0xFF1F2937)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          Row(
            children: [
              Icon(icon, size: 12, color: accentColor),
              const SizedBox(width: 4),
              Expanded(
                child: Text(
                  title,
                  style: const TextStyle(fontSize: 8.5, fontWeight: FontWeight.bold, color: Color(0xFF94A3B8)),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
            ],
          ),
          const SizedBox(height: 3),
          Text(
            value,
            style: const TextStyle(fontSize: 12, fontWeight: FontWeight.w800, color: Colors.white),
            overflow: TextOverflow.ellipsis,
          ),
        ],
      ),
    );
  }


}

// ---------------------------------------------------------------------------
// SCREEN 2: DAILY PRODUCTION ANALYTICS (MILITARY & INDUSTRIAL GRADE)
// ---------------------------------------------------------------------------
class DailyProductionScreen extends StatefulWidget {
  final InverterTelemetry telemetry;
  const DailyProductionScreen({super.key, required this.telemetry});

  @override
  State<DailyProductionScreen> createState() => _DailyProductionScreenState();
}

class _DailyProductionScreenState extends State<DailyProductionScreen> {
  int _selectedPeriod = 0; // 0: Today (24h), 1: Past 7 Days

  @override
  Widget build(BuildContext context) {
    final t = widget.telemetry;

    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 14.0, vertical: 10.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Header
            const Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Daily Solar Production',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.w800, color: Colors.white),
                ),
                Text(
                  'High-Precision Telemetry Curve',
                  style: TextStyle(fontSize: 11, color: Color(0xFF94A3B8)),
                ),
              ],
            ),

            const SizedBox(height: 10),

            // Period Selector Chips
            Container(
              padding: const EdgeInsets.all(4),
              decoration: BoxDecoration(
                color: const Color(0xFF111827),
                borderRadius: BorderRadius.circular(12),
                border: Border.all(color: const Color(0xFF1F2937)),
              ),
              child: Row(
                children: [
                  _buildPeriodTab(0, 'Today (24h)'),
                  _buildPeriodTab(1, 'Past 7 Days'),
                ],
              ),
            ),

            const SizedBox(height: 10),

            // Toggleable Fixed Non-Scrolling View
            if (_selectedPeriod == 0) ...[
              // Main Graph Container (24h Spline)
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: const Color(0xFF111827),
                  borderRadius: BorderRadius.circular(18),
                  border: Border.all(color: const Color(0xFF1F2937)),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Row(
                          children: [
                            const CircleAvatar(radius: 4, backgroundColor: Color(0xFFF59E0B)),
                            const SizedBox(width: 6),
                            const Text('Solar PV (W)', style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: Colors.white)),
                            const SizedBox(width: 14),
                            CircleAvatar(radius: 4, backgroundColor: const Color(0xFF38BDF8).withValues(alpha: 0.8)),
                            const SizedBox(width: 6),
                            const Text('AC Load (W)', style: TextStyle(fontSize: 11, color: Color(0xFF94A3B8))),
                          ],
                        ),
                        Text(
                          'Peak: ${math.max(t.solarWatts, 2520.0).round()} W @ 12:30',
                          style: const TextStyle(fontSize: 10, fontWeight: FontWeight.bold, color: Color(0xFFF59E0B)),
                        ),
                      ],
                    ),
                    const SizedBox(height: 10),

                    // Custom Spline Curve Canvas
                    SizedBox(
                      height: 165,
                      width: double.infinity,
                      child: CustomPaint(
                        painter: _SolarCurvePainter(
                          currentWatts: t.solarWatts,
                          loadWatts: t.loadWatts,
                        ),
                      ),
                    ),
                    const SizedBox(height: 6),

                    // Time axis labels
                    const Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Text('06:00', style: TextStyle(fontSize: 9.5, color: Color(0xFF64748B))),
                        Text('09:00', style: TextStyle(fontSize: 9.5, color: Color(0xFF64748B))),
                        Text('12:00', style: TextStyle(fontSize: 9.5, fontWeight: FontWeight.bold, color: Color(0xFFF59E0B))),
                        Text('15:00', style: TextStyle(fontSize: 9.5, color: Color(0xFF64748B))),
                        Text('18:00', style: TextStyle(fontSize: 9.5, color: Color(0xFF64748B))),
                        Text('21:00', style: TextStyle(fontSize: 9.5, color: Color(0xFF64748B))),
                      ],
                    ),
                  ],
                ),
              ),

              const SizedBox(height: 10),

              // 4 Industrial Standard KPIs (2x2)
              Row(
                children: [
                  Expanded(
                    child: _buildKpiCard(
                      title: "DAILY YIELD",
                      value: "${t.todayKwh} kWh",
                      subtext: "92.5% of Target",
                      subtextColor: const Color(0xFF10B981),
                      icon: CupertinoIcons.sun_max_fill,
                      color: const Color(0xFFF59E0B),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: _buildKpiCard(
                      title: "SPECIFIC YIELD",
                      value: "4.62 kWh/kWp",
                      subtext: "Class A Rating",
                      subtextColor: const Color(0xFF38BDF8),
                      icon: CupertinoIcons.chart_pie_fill,
                      color: const Color(0xFF38BDF8),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              Row(
                children: [
                  Expanded(
                    child: _buildKpiCard(
                      title: "PERFORMANCE RATIO",
                      value: "84.2%",
                      subtext: "Optimal Efficiency",
                      subtextColor: const Color(0xFF10B981),
                      icon: CupertinoIcons.speedometer,
                      color: const Color(0xFF10B981),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: _buildKpiCard(
                      title: "CO2 AVOIDED",
                      value: "11.8 kg",
                      subtext: "Today's Clean Yield",
                      subtextColor: const Color(0xFF34D399),
                      icon: CupertinoIcons.leaf_arrow_circlepath,
                      color: const Color(0xFF34D399),
                    ),
                  ),
                ],
              ),
            ] else ...[
              // 7-Day Production Bar Chart
              Container(
                padding: const EdgeInsets.all(14),
                decoration: BoxDecoration(
                  color: const Color(0xFF111827),
                  borderRadius: BorderRadius.circular(18),
                  border: Border.all(color: const Color(0xFF1F2937)),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Text(
                          '7-DAY PRODUCTION TREND (kWh)',
                          style: TextStyle(fontSize: 10.5, fontWeight: FontWeight.bold, color: Color(0xFF94A3B8)),
                        ),
                        Text(
                          'TOTAL: 98.4 kWh',
                          style: TextStyle(fontSize: 10.5, fontWeight: FontWeight.bold, color: Color(0xFFF59E0B)),
                        ),
                      ],
                    ),
                    const SizedBox(height: 18),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceAround,
                      crossAxisAlignment: CrossAxisAlignment.end,
                      children: [
                        _buildBar('Mon', 12.4, 0.65),
                        _buildBar('Tue', 14.1, 0.75),
                        _buildBar('Wed', 16.8, 0.90),
                        _buildBar('Thu', 11.2, 0.55),
                        _buildBar('Fri', 15.0, 0.80),
                        _buildBar('Sat', 17.5, 0.95),
                        _buildBar('Sun', t.todayKwh, 0.78, isToday: true),
                      ],
                    ),
                  ],
                ),
              ),

              const SizedBox(height: 10),

              // 4 Weekly Summary KPIs (2x2)
              Row(
                children: [
                  Expanded(
                    child: _buildKpiCard(
                      title: "WEEKLY YIELD",
                      value: "98.4 kWh",
                      subtext: "14.1 kWh/day avg",
                      subtextColor: const Color(0xFF10B981),
                      icon: CupertinoIcons.sun_max_fill,
                      color: const Color(0xFFF59E0B),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: _buildKpiCard(
                      title: "PEAK DAY",
                      value: "17.5 kWh",
                      subtext: "Saturday (Record)",
                      subtextColor: const Color(0xFFF59E0B),
                      icon: CupertinoIcons.bolt_fill,
                      color: const Color(0xFF38BDF8),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              Row(
                children: [
                  Expanded(
                    child: _buildKpiCard(
                      title: "SELF-USE RATIO",
                      value: "86.4%",
                      subtext: "High Grid Autonomy",
                      subtextColor: const Color(0xFF10B981),
                      icon: CupertinoIcons.speedometer,
                      color: const Color(0xFF10B981),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: _buildKpiCard(
                      title: "CARBON OFFSET",
                      value: "78.7 kg",
                      subtext: "Equivalent to 3 Trees",
                      subtextColor: const Color(0xFF34D399),
                      icon: CupertinoIcons.leaf_arrow_circlepath,
                      color: const Color(0xFF34D399),
                    ),
                  ),
                ],
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildPeriodTab(int index, String label) {
    final isSelected = _selectedPeriod == index;
    return Expanded(
      child: GestureDetector(
        onTap: () => setState(() => _selectedPeriod = index),
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 8),
          decoration: BoxDecoration(
            color: isSelected ? const Color(0xFF1F2937) : Colors.transparent,
            borderRadius: BorderRadius.circular(9),
          ),
          alignment: Alignment.center,
          child: Text(
            label,
            style: TextStyle(
              fontSize: 11,
              fontWeight: isSelected ? FontWeight.bold : FontWeight.w500,
              color: isSelected ? Colors.white : const Color(0xFF94A3B8),
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildKpiCard({
    required String title,
    required String value,
    required String subtext,
    required Color subtextColor,
    required IconData icon,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: const Color(0xFF111827),
        borderRadius: BorderRadius.circular(14),
        border: Border.all(color: const Color(0xFF1F2937)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(title, style: const TextStyle(fontSize: 9.5, fontWeight: FontWeight.bold, color: Color(0xFF94A3B8))),
              Icon(icon, size: 14, color: color),
            ],
          ),
          const SizedBox(height: 6),
          Text(value, style: const TextStyle(fontSize: 16, fontWeight: FontWeight.w800, color: Colors.white)),
          const SizedBox(height: 2),
          Text(subtext, style: TextStyle(fontSize: 9.5, fontWeight: FontWeight.w600, color: subtextColor)),
        ],
      ),
    );
  }

  Widget _buildBar(String day, double kwh, double heightFactor, {bool isToday = false}) {
    return Column(
      children: [
        Text(
          kwh.toStringAsFixed(1),
          style: TextStyle(fontSize: 9.5, fontWeight: FontWeight.bold, color: isToday ? const Color(0xFFF59E0B) : Colors.white60),
        ),
        const SizedBox(height: 6),
        Container(
          width: 24,
          height: 100 * heightFactor,
          decoration: BoxDecoration(
            gradient: LinearGradient(
              colors: isToday
                  ? [const Color(0xFFF59E0B), const Color(0xFFD97706)]
                  : [const Color(0xFF38BDF8), const Color(0xFF0284C7)],
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
            ),
            borderRadius: BorderRadius.circular(5),
          ),
        ),
        const SizedBox(height: 6),
        Text(
          day,
          style: TextStyle(
            fontSize: 10,
            fontWeight: isToday ? FontWeight.bold : FontWeight.normal,
            color: isToday ? const Color(0xFFF59E0B) : Colors.white60,
          ),
        ),
      ],
    );
  }
}

class _SolarCurvePainter extends CustomPainter {
  final double currentWatts;
  final double loadWatts;

  _SolarCurvePainter({required this.currentWatts, required this.loadWatts});

  @override
  void paint(Canvas canvas, Size size) {
    final w = size.width;
    final h = size.height;

    // 1. Draw subtle horizontal grid lines (0W, 1000W, 2000W, 3000W)
    final gridPaint = Paint()
      ..color = const Color(0xFF1E293B)
      ..strokeWidth = 1.0;

    for (int i = 0; i <= 3; i++) {
      final y = h * (1.0 - i / 3.0);
      canvas.drawLine(Offset(0, y), Offset(w, y), gridPaint);
    }

    // Solar curve points
    final points = [
      Offset(0, h),
      Offset(w * 0.10, h * (1.0 - 150 / 3000)),
      Offset(w * 0.25, h * (1.0 - 950 / 3000)),
      Offset(w * 0.40, h * (1.0 - 2150 / 3000)),
      Offset(w * 0.50, h * (1.0 - 2680 / 3000)),
      Offset(w * 0.65, h * (1.0 - 2350 / 3000)),
      Offset(w * 0.80, h * (1.0 - 1200 / 3000)),
      Offset(w * 0.92, h * (1.0 - 200 / 3000)),
      Offset(w, h),
    ];

    final path = Path()..moveTo(points[0].dx, points[0].dy);
    for (int i = 0; i < points.length - 1; i++) {
      final p0 = points[i];
      final p1 = points[i + 1];
      final cx = (p0.dx + p1.dx) / 2;
      path.cubicTo(cx, p0.dy, cx, p1.dy, p1.dx, p1.dy);
    }

    // Fill under curve
    final fillPath = Path.from(path)
      ..lineTo(w, h)
      ..lineTo(0, h)
      ..close();

    final fillPaint = Paint()
      ..shader = const LinearGradient(
        colors: [Color(0x55F59E0B), Color(0x00F59E0B)],
        begin: Alignment.topCenter,
        end: Alignment.bottomCenter,
      ).createShader(Rect.fromLTWH(0, 0, w, h));

    canvas.drawPath(fillPath, fillPaint);

    // Stroke curve
    final strokePaint = Paint()
      ..color = const Color(0xFFF59E0B)
      ..strokeWidth = 2.5
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round;

    canvas.drawPath(path, strokePaint);

    // Household Load line (Dashed cyan line)
    final loadY = (h * (1.0 - (loadWatts.clamp(200.0, 2800.0) / 3000.0)));
    final loadPaint = Paint()
      ..color = const Color(0xFF38BDF8).withValues(alpha: 0.6)
      ..strokeWidth = 1.5
      ..style = PaintingStyle.stroke;

    double startX = 0;
    while (startX < w) {
      canvas.drawLine(Offset(startX, loadY), Offset(startX + 6, loadY), loadPaint);
      startX += 12;
    }

    // Current Generation Marker (pulsing dot at w * 0.45)
    final markerX = w * 0.45;
    final markerY = h * (1.0 - (currentWatts.clamp(50.0, 2900.0) / 3000.0));

    final pulsePaint = Paint()
      ..color = const Color(0xFFF59E0B).withValues(alpha: 0.35)
      ..style = PaintingStyle.fill;
    canvas.drawCircle(Offset(markerX, markerY), 7, pulsePaint);

    final dotPaint = Paint()..color = const Color(0xFFF59E0B);
    canvas.drawCircle(Offset(markerX, markerY), 3.5, dotPaint);
  }

  @override
  bool shouldRepaint(covariant _SolarCurvePainter oldDelegate) =>
      oldDelegate.currentWatts != currentWatts || oldDelegate.loadWatts != loadWatts;
}

// ---------------------------------------------------------------------------
// SCREEN 3: WIRELESS BLE WI-FI PROVISIONING
// ---------------------------------------------------------------------------
class ProvisioningScreen extends StatefulWidget {
  final InverterTelemetry telemetry;
  const ProvisioningScreen({super.key, required this.telemetry});

  @override
  State<ProvisioningScreen> createState() => _ProvisioningScreenState();
}

class _ProvisioningScreenState extends State<ProvisioningScreen> with WidgetsBindingObserver {
  static const _wifiChannel = MethodChannel('com.sungridnova/wifi');

  final _ssidController = TextEditingController();
  final _passController = TextEditingController();
  final _bleService = BleProvisioningService();

  bool _isScanning = false;
  bool _isTransmitting = false;
  bool _isPhoneWifiConnected = false;

  String? _topBannerText;
  Color _topBannerColor = const Color(0xFF10B981);
  IconData _topBannerIcon = CupertinoIcons.check_mark_circled;
  Timer? _bannerTimer;

  String _statusMsg = 'Tap "Scan Inverter" to detect nearby SunGridNova hardware';
  final List<BluetoothDevice> _devices = [];
  BluetoothDevice? _selectedDevice;

  StreamSubscription? _msgSub;
  StreamSubscription? _devSub;
  StreamSubscription? _provSub;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _fetchPhoneWifi();

    _msgSub = _bleService.statusMessageStream.listen((msg) {
      if (!mounted) return;
      setState(() => _statusMsg = msg);
    });

    _devSub = _bleService.deviceFoundStream.listen((dev) {
      if (!mounted) return;
      if (!_devices.any((d) => d.remoteId == dev.remoteId)) {
        setState(() {
          _devices.add(dev);
          _selectedDevice ??= dev;
        });
      }
    });

    _provSub = _bleService.isProvisionedStream.listen((ok) {
      if (!mounted) return;
      if (ok) {
        _showTopBanner(
          text: 'Inverter verified Wi-Fi! Live sync with Cloud Server active.',
          color: const Color(0xFF10B981),
          icon: CupertinoIcons.checkmark_seal_fill,
          durationSec: 4,
        );
      }
    });
  }

  Future<void> _fetchPhoneWifi() async {
    try {
      var res = await _wifiChannel.invokeMethod<Map<dynamic, dynamic>>('getConnectedWifiInfo');
      if (res != null && res['isConnected'] == true && (res['ssid'] == null || (res['ssid'] as String).isEmpty)) {
        await Future.delayed(const Duration(milliseconds: 600));
        res = await _wifiChannel.invokeMethod<Map<dynamic, dynamic>>('getConnectedWifiInfo');
      }
      if (res != null) {
        final bool connected = res['isConnected'] == true;
        final String? ssid = res['ssid'] as String?;
        final bool isLocationEnabled = res['isLocationEnabled'] ?? true;
        if (mounted) {
          setState(() {
            _isPhoneWifiConnected = connected;
            if (connected && ssid != null && ssid.isNotEmpty) {
              _ssidController.text = ssid;
            } else if (!connected) {
              _ssidController.text = '';
            }
          });
          if (connected && (ssid == null || ssid.isEmpty) && !isLocationEnabled) {
            _showTopBanner(
              text: 'Please turn on Location (GPS) in phone quick settings to auto-fill Wi-Fi name',
              color: const Color(0xFFF59E0B),
              icon: CupertinoIcons.location_slash_fill,
              durationSec: 5,
            );
          }
        }
      }
    } catch (e) {
      debugPrint('Error getting phone Wi-Fi: $e');
    }
  }

  void _showTopBanner({
    required String text,
    required Color color,
    required IconData icon,
    int durationSec = 3,
  }) {
    _bannerTimer?.cancel();
    setState(() {
      _topBannerText = text;
      _topBannerColor = color;
      _topBannerIcon = icon;
    });
    _bannerTimer = Timer(Duration(seconds: durationSec), () {
      if (mounted) {
        setState(() {
          _topBannerText = null;
        });
      }
    });
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      _fetchPhoneWifi();
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _bannerTimer?.cancel();
    _msgSub?.cancel();
    _devSub?.cancel();
    _provSub?.cancel();
    _ssidController.dispose();
    _passController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      child: ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          // Top Transient Status Banner (auto-vanishes, non-blocking)
          AnimatedCrossFade(
            duration: const Duration(milliseconds: 250),
            crossFadeState: _topBannerText != null ? CrossFadeState.showFirst : CrossFadeState.showSecond,
            firstChild: _topBannerText != null
                ? Container(
                    margin: const EdgeInsets.only(bottom: 12),
                    padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                    decoration: BoxDecoration(
                      color: _topBannerColor.withValues(alpha: 0.18),
                      borderRadius: BorderRadius.circular(12),
                      border: Border.all(color: _topBannerColor, width: 1.2),
                    ),
                    child: Row(
                      children: [
                        Icon(_topBannerIcon, color: _topBannerColor, size: 20),
                        const SizedBox(width: 10),
                        Expanded(
                          child: Text(
                            _topBannerText ?? '',
                            style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold, color: _topBannerColor),
                          ),
                        ),
                      ],
                    ),
                  )
                : const SizedBox.shrink(),
            secondChild: const SizedBox.shrink(),
          ),

          // Phone Wi-Fi Status Card (only displayed if phone is not connected to Wi-Fi)
          if (!_isPhoneWifiConnected)
            Container(
              margin: const EdgeInsets.only(bottom: 14),
              padding: const EdgeInsets.all(14),
              decoration: BoxDecoration(
                color: const Color(0xFF7F1D1D).withValues(alpha: 0.35),
                borderRadius: BorderRadius.circular(14),
                border: Border.all(color: const Color(0xFFEF4444), width: 1.2),
              ),
              child: Row(
                children: [
                  const Icon(CupertinoIcons.exclamationmark_triangle_fill, color: Color(0xFFEF4444), size: 22),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: const [
                        Text(
                          'PHONE NOT CONNECTED TO WI-FI',
                          style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: Color(0xFFEF4444)),
                        ),
                        SizedBox(height: 2),
                        Text(
                          'Please connect this phone to your 2.4 GHz Wi-Fi network before provisioning.',
                          style: TextStyle(fontSize: 11, color: Color(0xFFFECACA)),
                        ),
                      ],
                    ),
                  ),
                  IconButton(
                    icon: const Icon(CupertinoIcons.arrow_clockwise, color: Color(0xFFEF4444), size: 18),
                    onPressed: _fetchPhoneWifi,
                    tooltip: 'Re-check Phone Wi-Fi',
                  ),
                ],
              ),
            ),

          const Text(
            'Wireless Inverter Setup',
            style: TextStyle(fontSize: 22, fontWeight: FontWeight.w800, color: Colors.white),
          ),
          const SizedBox(height: 6),
          const Text(
            'Bluetooth (BLE) is only used once to push Wi-Fi credentials. Telemetry syncs continuously with Cloud Server.',
            style: TextStyle(fontSize: 12, color: Color(0xFF94A3B8)),
          ),
          const SizedBox(height: 14),

          const SizedBox(height: 14),

          // Inverter Hardware Live Wi-Fi Status Card

          // 2. Inverter Hardware Live Wi-Fi Status Card
          Container(
            margin: const EdgeInsets.only(bottom: 16),
            padding: const EdgeInsets.all(14),
            decoration: BoxDecoration(
              color: const Color(0xFF111827),
              borderRadius: BorderRadius.circular(16),
              border: Border.all(
                color: (_bleService.inverterIp != null && _bleService.inverterIp != '0.0.0.0') ||
                        (widget.telemetry.ipAddress.isNotEmpty && widget.telemetry.ipAddress != '0.0.0.0')
                    ? const Color(0xFF10B981)
                    : const Color(0xFFF59E0B),
                width: 1.2,
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text(
                      'INVERTER WI-FI HARDWARE STATUS',
                      style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: Color(0xFFF59E0B)),
                    ),
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
                      decoration: BoxDecoration(
                        color: (_bleService.inverterIp != null && _bleService.inverterIp != '0.0.0.0') ||
                                (widget.telemetry.ipAddress.isNotEmpty && widget.telemetry.ipAddress != '0.0.0.0')
                            ? const Color(0xFF064E3B)
                            : const Color(0xFF78350F),
                        borderRadius: BorderRadius.circular(6),
                      ),
                      child: Text(
                        (_bleService.inverterIp != null && _bleService.inverterIp != '0.0.0.0') ||
                                (widget.telemetry.ipAddress.isNotEmpty && widget.telemetry.ipAddress != '0.0.0.0')
                            ? 'CONNECTED'
                            : 'DISCONNECTED',
                        style: TextStyle(
                          fontSize: 10,
                          fontWeight: FontWeight.bold,
                          color: (_bleService.inverterIp != null && _bleService.inverterIp != '0.0.0.0') ||
                                  (widget.telemetry.ipAddress.isNotEmpty && widget.telemetry.ipAddress != '0.0.0.0')
                              ? const Color(0xFF34D399)
                              : const Color(0xFFFBBF24),
                        ),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 10),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text('Inverter Active SSID:', style: TextStyle(fontSize: 12, color: Color(0xFF94A3B8))),
                    Text(
                      _bleService.inverterSsid ??
                          (widget.telemetry.connectedSsid.isNotEmpty ? widget.telemetry.connectedSsid : 'Not Connected'),
                      style: const TextStyle(fontSize: 13, fontWeight: FontWeight.bold, color: Colors.white),
                    ),
                  ],
                ),
                const SizedBox(height: 4),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text('Inverter IP Address:', style: TextStyle(fontSize: 12, color: Color(0xFF94A3B8))),
                    Text(
                      _bleService.inverterIp ??
                          (widget.telemetry.ipAddress.isNotEmpty ? widget.telemetry.ipAddress : '0.0.0.0'),
                      style: const TextStyle(fontSize: 12, fontWeight: FontWeight.bold, color: Color(0xFF38BDF8)),
                    ),
                  ],
                ),
                if (_bleService.deviceMac != null || widget.telemetry.thingId.isNotEmpty) ...[
                  const SizedBox(height: 4),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      const Text('Inverter Hardware ID:', style: TextStyle(fontSize: 12, color: Color(0xFF94A3B8))),
                      Text(
                        _bleService.deviceMac != null ? 'MAC: ${_bleService.deviceMac}' : widget.telemetry.thingId,
                        style: const TextStyle(fontSize: 11, fontFamily: 'monospace', color: Colors.white70),
                      ),
                    ],
                  ),
                ],
              ],
            ),
          ),

          // Scanner Card
          Container(
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              color: const Color(0xFF111827),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: const Color(0xFF1F2937)),
            ),
            child: Column(
              children: [
                ElevatedButton.icon(
                  onPressed: _isScanning
                      ? null
                      : () async {
                          setState(() {
                            _isScanning = true;
                            _devices.clear();
                          });
                          await _bleService.startScan();
                          Future.delayed(const Duration(seconds: 10), () {
                            if (mounted) setState(() => _isScanning = false);
                          });
                        },
                  icon: _isScanning
                      ? const SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.black))
                      : const Icon(CupertinoIcons.bluetooth, color: Colors.black),
                  label: Text(
                    _isScanning ? 'SCANNING...' : 'SCAN FOR INVERTER',
                    style: const TextStyle(fontWeight: FontWeight.w800, color: Colors.black),
                  ),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF38BDF8),
                    padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 14),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                  ),
                ),
                const SizedBox(height: 12),
                Text(
                  _statusMsg,
                  textAlign: TextAlign.center,
                  style: const TextStyle(fontSize: 12, color: Color(0xFF94A3B8)),
                ),

                // Discovered Inverters List
                if (_devices.isNotEmpty) ...[
                  const SizedBox(height: 16),
                  const Divider(color: Color(0xFF1F2937)),
                  const SizedBox(height: 8),
                  const Align(
                    alignment: Alignment.centerLeft,
                    child: Text(
                      'DETECTED SUNGRIDNOVA HARDWARE:',
                      style: TextStyle(fontSize: 10, fontWeight: FontWeight.bold, color: Color(0xFFF59E0B)),
                    ),
                  ),
                  const SizedBox(height: 8),
                  ..._devices.map((d) {
                    final isSelected = _selectedDevice?.remoteId == d.remoteId;
                    return InkWell(
                      onTap: () async {
                        setState(() => _selectedDevice = d);
                        await _bleService.connect(d);
                      },
                      child: Container(
                        margin: const EdgeInsets.symmetric(vertical: 4),
                        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                        decoration: BoxDecoration(
                          color: isSelected ? const Color(0xFF0F2942) : const Color(0xFF1E293B),
                          borderRadius: BorderRadius.circular(12),
                          border: Border.all(
                            color: isSelected ? const Color(0xFF38BDF8) : Colors.transparent,
                            width: 1.5,
                          ),
                        ),
                        child: Row(
                          mainAxisAlignment: MainAxisAlignment.spaceBetween,
                          children: [
                            Row(
                              children: [
                                const Icon(CupertinoIcons.antenna_radiowaves_left_right, color: Color(0xFF38BDF8), size: 18),
                                const SizedBox(width: 10),
                                Text(
                                  d.advName.isNotEmpty ? d.advName : 'SunGridNova Device',
                                  style: const TextStyle(fontWeight: FontWeight.w700, color: Colors.white),
                                ),
                              ],
                            ),
                            Text(
                              isSelected ? 'CONNECTED' : 'TAP TO PAIR',
                              style: TextStyle(
                                fontSize: 10,
                                fontWeight: FontWeight.bold,
                                color: isSelected ? const Color(0xFF34D399) : const Color(0xFF94A3B8),
                              ),
                            ),
                          ],
                        ),
                      ),
                    );
                  }),
                ],
              ],
            ),
          ),

          const SizedBox(height: 20),

          // Wi-Fi Credentials Input
          Container(
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              color: const Color(0xFF111827),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: const Color(0xFF1F2937)),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Text(
                  'WI-FI CREDENTIALS',
                  style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: Color(0xFF94A3B8)),
                ),
                const SizedBox(height: 14),
                TextField(
                  controller: _ssidController,
                  decoration: InputDecoration(
                    labelText: 'Wi-Fi Network Name (SSID)',
                    prefixIcon: const Icon(CupertinoIcons.wifi, color: Color(0xFFF59E0B)),
                    suffixIcon: IconButton(
                      icon: const Icon(CupertinoIcons.arrow_clockwise, color: Color(0xFFF59E0B), size: 18),
                      tooltip: 'Auto-detect Phone Wi-Fi',
                      onPressed: _fetchPhoneWifi,
                    ),
                    border: OutlineInputBorder(borderRadius: BorderRadius.circular(12)),
                    filled: true,
                    fillColor: const Color(0xFF1E293B),
                  ),
                ),
                const SizedBox(height: 14),
                TextField(
                  controller: _passController,
                  obscureText: true,
                  decoration: InputDecoration(
                    labelText: 'Wi-Fi Password',
                    prefixIcon: const Icon(CupertinoIcons.lock_fill, color: Color(0xFFF59E0B)),
                    border: OutlineInputBorder(borderRadius: BorderRadius.circular(12)),
                    filled: true,
                    fillColor: const Color(0xFF1E293B),
                  ),
                ),
                const SizedBox(height: 18),
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton(
                    onPressed: (!_isPhoneWifiConnected || _isTransmitting)
                        ? null
                        : () async {
                            final ssid = _ssidController.text.trim();
                            final pass = _passController.text.trim();
                            if (ssid.isEmpty) {
                              _showTopBanner(
                                text: 'Please enter Wi-Fi Network Name (SSID)',
                                color: const Color(0xFFEF4444),
                                icon: CupertinoIcons.exclamationmark_circle,
                              );
                              return;
                            }

                            setState(() => _isTransmitting = true);
                            _showTopBanner(
                              text: 'Transmitting credentials to inverter via BLE...',
                              color: const Color(0xFFF59E0B),
                              icon: CupertinoIcons.arrow_2_circlepath,
                              durationSec: 2,
                            );

                            // If not yet connected over BLE, connect to selected device first
                            if (!_bleService.isConnected) {
                              if (_selectedDevice != null) {
                                final ok = await _bleService.connect(_selectedDevice!);
                                if (!mounted) return;
                                if (!ok) {
                                  setState(() => _isTransmitting = false);
                                  _showTopBanner(
                                    text: 'Failed to connect to inverter over Bluetooth',
                                    color: const Color(0xFFEF4444),
                                    icon: CupertinoIcons.xmark_circle,
                                  );
                                  return;
                                }
                              } else {
                                setState(() => _isTransmitting = false);
                                _showTopBanner(
                                  text: 'Please scan and select a SunGridNova device first',
                                  color: const Color(0xFFEF4444),
                                  icon: CupertinoIcons.exclamationmark_circle,
                                );
                                return;
                              }
                            }

                            final success = await _bleService.sendCredentials(ssid: ssid, password: pass);
                            if (!mounted) return;
                            setState(() => _isTransmitting = false);
                            if (success) {
                              _showTopBanner(
                                text: 'Credentials sent! Inverter connecting to "$ssid"...',
                                color: const Color(0xFF10B981),
                                icon: CupertinoIcons.checkmark_circle_fill,
                                durationSec: 4,
                              );
                            } else {
                              _showTopBanner(
                                text: 'Failed to transmit credentials to inverter',
                                color: const Color(0xFFEF4444),
                                icon: CupertinoIcons.xmark_circle,
                              );
                            }
                          },
                    style: ElevatedButton.styleFrom(
                      backgroundColor: _isPhoneWifiConnected ? const Color(0xFFF59E0B) : const Color(0xFF475569),
                      disabledBackgroundColor: const Color(0xFF334155),
                      padding: const EdgeInsets.symmetric(vertical: 14),
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                    ),
                    child: _isTransmitting
                        ? const SizedBox(
                            width: 20,
                            height: 20,
                            child: CircularProgressIndicator(strokeWidth: 2, color: Colors.black),
                          )
                        : Text(
                            _isPhoneWifiConnected
                                ? 'PROVISION WI-FI TO INVERTER'
                                : 'CONNECT PHONE TO WI-FI FIRST',
                            style: TextStyle(
                              fontWeight: FontWeight.w800,
                              color: _isPhoneWifiConnected ? Colors.black : const Color(0xFF94A3B8),
                            ),
                          ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// SCREEN 4: EXECUTIVE OEM VENDOR PROFILE & COMPLIANCE
// ---------------------------------------------------------------------------
class VendorScreen extends StatelessWidget {
  final InverterTelemetry telemetry;
  const VendorScreen({super.key, required this.telemetry});

  @override
  Widget build(BuildContext context) {
    final vName = telemetry.vendorName.isNotEmpty ? telemetry.vendorName : 'OEM Solar Unit';
    final mName = telemetry.modelName.isNotEmpty ? telemetry.modelName : 'Hybrid MPPT Controller';
    final sNum = telemetry.serialNumber.isNotEmpty
        ? telemetry.serialNumber
        : (telemetry.thingId.isNotEmpty ? telemetry.thingId : 'Solar-Unit');
    final hVer = telemetry.hardwareVersion.isNotEmpty ? telemetry.hardwareVersion : 'HW-V2.1';
    final vContact = telemetry.vendorContact.isNotEmpty ? telemetry.vendorContact : 'Support Portal';
    final vSite = telemetry.vendorWebsite.isNotEmpty ? telemetry.vendorWebsite : 'Customer Care Portal';

    return SafeArea(
      child: ListView(
        padding: const EdgeInsets.symmetric(horizontal: 16.0, vertical: 14.0),
        children: [
          // 1. Top Emblem Branding Card
          Container(
            padding: const EdgeInsets.all(20),
            decoration: BoxDecoration(
              gradient: const LinearGradient(
                colors: [Color(0xFF1E293B), Color(0xFF0F172A)],
                begin: Alignment.topLeft,
                end: Alignment.bottomRight,
              ),
              borderRadius: BorderRadius.circular(24),
              border: Border.all(color: const Color(0xFF334155), width: 1.2),
              boxShadow: [
                BoxShadow(
                  color: const Color(0xFFF59E0B).withValues(alpha: 0.15),
                  blurRadius: 20,
                  offset: const Offset(0, 6),
                ),
              ],
            ),
            child: Column(
              children: [
                // Glowing Emblem Container with OEM Logo
                Container(
                  width: 84,
                  height: 84,
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: const Color(0xFF0B1120),
                    border: Border.all(color: const Color(0xFFF59E0B), width: 2),
                    boxShadow: [
                      BoxShadow(
                        color: const Color(0xFFF59E0B).withValues(alpha: 0.35),
                        blurRadius: 16,
                        spreadRadius: 2,
                      ),
                    ],
                  ),
                  child: Image.asset(
                    'assets/logo.png',
                    fit: BoxFit.contain,
                    errorBuilder: (ctx, err, stack) => const Icon(
                      CupertinoIcons.sun_max_fill,
                      color: Color(0xFFF59E0B),
                      size: 48,
                    ),
                  ),
                ),
                const SizedBox(height: 14),
                Text(
                  vName,
                  style: const TextStyle(
                    fontSize: 22,
                    fontWeight: FontWeight.w900,
                    letterSpacing: 1.0,
                    color: Color(0xFFF59E0B),
                  ),
                  textAlign: TextAlign.center,
                ),
                const SizedBox(height: 4),
                Text(
                  mName,
                  style: const TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.w600,
                    color: Colors.white,
                  ),
                  textAlign: TextAlign.center,
                ),
                const SizedBox(height: 12),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 5),
                  decoration: BoxDecoration(
                    color: const Color(0xFF064E3B).withValues(alpha: 0.8),
                    borderRadius: BorderRadius.circular(20),
                    border: Border.all(color: const Color(0xFF10B981), width: 1),
                  ),
                  child: const Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(CupertinoIcons.checkmark_seal_fill, color: Color(0xFF34D399), size: 14),
                      SizedBox(width: 6),
                      Text(
                        'ISO 9001 / IEC 62109 COMPATIBLE',
                        style: TextStyle(
                          fontSize: 10,
                          fontWeight: FontWeight.w800,
                          letterSpacing: 0.5,
                          color: Color(0xFF34D399),
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),

          const SizedBox(height: 16),

          // 2. Card 1: OEM System Identification
          _buildVendorCard(
            title: 'OEM SYSTEM IDENTIFICATION',
            icon: CupertinoIcons.info_circle_fill,
            items: [
              _buildRow('Manufacturer / Brand', vName),
              _buildRow('Product Model', mName),
              _buildRow('Serial Number', sNum),
              _buildRow('Hardware Revision', hVer),
              _buildRow('Display Subsystem', '320x240 TrueColor TFT LCD'),
              _buildRow('Processing Unit', 'ESP32-S3 Dual-Core (16MB Flash, 8MB PSRAM)'),
            ],
          ),

          const SizedBox(height: 14),

          // 3. Card 2: Standards & Industrial Compatibility
          _buildVendorCard(
            title: 'REGULATORY & INDUSTRIAL COMPATIBILITY',
            icon: CupertinoIcons.shield_lefthalf_fill,
            items: [
              _buildRow('Quality Management', 'ISO 9001:2015 Compatible'),
              _buildRow('Electrical Safety', 'IEC 62109-1 / IEC 62109-2 Compatible'),
              _buildRow('Environmental Durability', 'MIL-STD-810H Compatible'),
              _buildRow('Electromagnetic Conformity', 'CE & RoHS Compatible'),
            ],
          ),

          const SizedBox(height: 14),

          // 4. Card 3: Support & Official Portal
          _buildVendorCard(
            title: 'CUSTOMER CARE & TECHNICAL SUPPORT',
            icon: CupertinoIcons.phone_fill,
            items: [
              _buildRow('Helpline / Toll-Free', vContact),
              _buildRow('Official Web Portal', vSite),
              _buildRow('Technical Assistance', '24x7 Industrial Engineering Support'),
              _buildRow('Warranty Validation', 'OEM Active Commercial Protection'),
            ],
          ),

          const SizedBox(height: 14),

          // 5. Card 4: Controller Network & Cloud Link
          _buildVendorCard(
            title: 'CONTROLLER CLOUD & NETWORK STATUS',
            icon: CupertinoIcons.cloud_fill,
            items: [
              _buildRow('Device Identity', telemetry.thingId),
              _buildRow('Wi-Fi Network', telemetry.connectedSsid.isNotEmpty ? telemetry.connectedSsid : 'Online'),
              _buildRow('Controller IP', telemetry.ipAddress.isNotEmpty ? telemetry.ipAddress : '0.0.0.0'),
              _buildRow(
                'IoT Cloud Stream',
                telemetry.isLiveAws
                    ? 'CONNECTED (Live AWS IoT Core)'
                    : (telemetry.awsStatus == AwsConnectionStatus.connecting
                        ? 'CONNECTING...'
                        : 'OFFLINE'),
              ),
              _buildRow('Transport Security', 'Encrypted TLS 1.2 (Hardware Root of Trust)'),
            ],
          ),
          const SizedBox(height: 20),
        ],
      ),
    );
  }

  Widget _buildVendorCard({
    required String title,
    required IconData icon,
    required List<Widget> items,
  }) {
    return Container(
      padding: const EdgeInsets.all(18),
      decoration: BoxDecoration(
        color: const Color(0xFF111827),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: const Color(0xFF1F2937)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(icon, color: const Color(0xFFF59E0B), size: 16),
              const SizedBox(width: 8),
              Text(
                title,
                style: const TextStyle(
                  fontSize: 11,
                  fontWeight: FontWeight.bold,
                  letterSpacing: 0.5,
                  color: Color(0xFFF59E0B),
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          ...items,
        ],
      ),
    );
  }

  Widget _buildRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 5.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: const TextStyle(fontSize: 12, color: Color(0xFF94A3B8)),
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              value,
              textAlign: TextAlign.end,
              overflow: TextOverflow.ellipsis,
              maxLines: 2,
              style: const TextStyle(
                fontSize: 12,
                fontWeight: FontWeight.w600,
                color: Colors.white,
              ),
            ),
          ),
        ],
      ),
    );
  }
}
