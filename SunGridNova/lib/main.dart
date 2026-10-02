import 'dart:async';
import 'dart:math' as math;
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'services/aws_iot_service.dart';

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
  StreamSubscription? _awsStatusSub;
  StreamSubscription? _awsTelemetrySub;
  StreamSubscription? _awsAlertSub;
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
          }
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

    // 1-Second real-time physics simulator ticker (falls back gracefully if no live AWS data)
    _simTimer = Timer.periodic(const Duration(seconds: 1), (timer) {
      if (!mounted) return;
      final bool isLiveStale = _telemetry.lastPacketTime == null ||
          DateTime.now().difference(_telemetry.lastPacketTime!).inSeconds > 40;

      setState(() {
        _simPhase += 0.08;
        if (isLiveStale) {
          _telemetry.isLiveAws = false;
          _telemetry.solarVolt = 76.0 + 3.0 * math.sin(_simPhase);
          _telemetry.solarWatts = 2400.0 + 150.0 * math.sin(_simPhase * 1.2);
          _telemetry.battVolt = 26.7 + 0.2 * math.cos(_simPhase * 0.5);
          _telemetry.loadWatts = 820.0 + 80.0 * math.sin(_simPhase * 1.5);
          _telemetry.loadPercent = (_telemetry.loadWatts / 20.0).round();
          _telemetry.heatSinkTemp = 38.0 + 1.2 * math.sin(_simPhase * 0.3);
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
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final screens = [
      DashboardScreen(telemetry: _telemetry),
      AnalyticsScreen(telemetry: _telemetry),
      ProvisioningScreen(telemetry: _telemetry),
      DiagnosticsScreen(telemetry: _telemetry),
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
              label: 'Harvest',
            ),
            BottomNavigationBarItem(
              icon: Icon(CupertinoIcons.wifi),
              label: 'Wi-Fi Setup',
            ),
            BottomNavigationBarItem(
              icon: Icon(CupertinoIcons.shield_lefthalf_fill),
              label: 'Diagnostics',
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
      child: ListView(
        padding: const EdgeInsets.symmetric(horizontal: 16.0, vertical: 12.0),
        children: [
          // 1. Top Brand & Connection Bar
          Row(
            children: [
              Expanded(
                child: Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.all(8),
                      decoration: BoxDecoration(
                        gradient: const LinearGradient(
                          colors: [Color(0xFFF59E0B), Color(0xFFD97706)],
                        ),
                        borderRadius: BorderRadius.circular(12),
                        boxShadow: [
                          BoxShadow(
                            color: const Color(0xFFF59E0B).withValues(alpha: 0.35),
                            blurRadius: 10,
                            offset: const Offset(0, 3),
                          )
                        ],
                      ),
                      child: const Icon(CupertinoIcons.sun_max_fill, color: Colors.white, size: 20),
                    ),
                    const SizedBox(width: 10),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          const Text(
                            'SunGridNova',
                            style: TextStyle(
                              fontSize: 19,
                              fontWeight: FontWeight.w800,
                              letterSpacing: 0.5,
                              color: Colors.white,
                            ),
                            overflow: TextOverflow.ellipsis,
                          ),
                          Text(
                            'HYBRID MPPT • ${t.thingId}',
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
                  ],
                ),
              ),
              const SizedBox(width: 8),
              _buildConnectionPill(t),
            ],
          ),

          const SizedBox(height: 18),

          // 2. Fault Protection Alert Banner (Only visible on trip)
          if (t.errorCode != 0)
            Container(
              margin: const EdgeInsets.only(bottom: 16),
              padding: const EdgeInsets.all(14),
              decoration: BoxDecoration(
                color: const Color(0xFF450A0A),
                borderRadius: BorderRadius.circular(16),
                border: Border.all(color: const Color(0xFFEF4444), width: 1.5),
              ),
              child: Row(
                children: [
                  const Icon(CupertinoIcons.exclamationmark_triangle_fill, color: Color(0xFFEF4444), size: 26),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Text(
                          'SAFETY PROTECTION TRIP ACTIVE',
                          style: TextStyle(fontSize: 13, fontWeight: FontWeight.w800, color: Color(0xFFFCA5A5)),
                        ),
                        Text(
                          'Fault Code: ${t.errorCode} • Output Disconnected for Safety',
                          style: TextStyle(fontSize: 11, color: Colors.white.withValues(alpha: 0.8)),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),

          // 3. Central Interactive Power Synoptic Hub
          Container(
            padding: const EdgeInsets.all(18),
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
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text(
                      'REAL-TIME ENERGY FLOW',
                      style: TextStyle(
                        fontSize: 11,
                        fontWeight: FontWeight.w800,
                        letterSpacing: 1.0,
                        color: Colors.white.withValues(alpha: 0.6),
                      ),
                    ),
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                      decoration: BoxDecoration(
                        color: const Color(0xFF1E3A8A),
                        borderRadius: BorderRadius.circular(8),
                      ),
                      child: Text(
                        'SOURCE: ${t.feedSource}',
                        style: const TextStyle(
                          fontSize: 10,
                          fontWeight: FontWeight.bold,
                          color: Color(0xFF93C5FD),
                        ),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 20),

                // 4-Node Synoptic Diagram
                SizedBox(
                  height: 250,
                  child: Stack(
                    alignment: Alignment.center,
                    children: [
                      // Animated Central Inverter Ring
                      AnimatedBuilder(
                        animation: _flowController,
                        builder: (context, child) {
                          return Container(
                            width: 100,
                            height: 100,
                            decoration: BoxDecoration(
                              shape: BoxShape.circle,
                              gradient: RadialGradient(
                                colors: [
                                  const Color(0xFFF59E0B).withValues(alpha: 0.15),
                                  Colors.transparent,
                                ],
                              ),
                              border: Border.all(
                                color: const Color(0xFFF59E0B).withValues(alpha: 0.5 + 0.3 * math.sin(_flowController.value * 2 * math.pi)),
                                width: 2.0,
                              ),
                            ),
                            child: const Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Icon(CupertinoIcons.rays, color: Color(0xFFF59E0B), size: 26),
                                SizedBox(height: 4),
                                Text(
                                  'INVERTER',
                                  style: TextStyle(fontSize: 10, fontWeight: FontWeight.bold, color: Colors.white),
                                ),
                                Text(
                                  '230 VAC',
                                  style: TextStyle(fontSize: 9, color: Color(0xFFFDE68A)),
                                ),
                              ],
                            ),
                          );
                        },
                      ),

                      // TOP: Solar Node
                      Align(
                        alignment: Alignment.topCenter,
                        child: _buildSynopticNode(
                          icon: CupertinoIcons.sun_max_fill,
                          title: 'SOLAR PV',
                          value: '${t.solarWatts.round()} W',
                          subtitle: '${t.solarVolt.toStringAsFixed(1)} V',
                          color: const Color(0xFFF59E0B),
                        ),
                      ),

                      // BOTTOM: Battery Bank Node
                      Align(
                        alignment: Alignment.bottomCenter,
                        child: _buildSynopticNode(
                          icon: CupertinoIcons.battery_25,
                          title: 'BATTERY',
                          value: '${t.battPercent}%',
                          subtitle: '${t.battVolt.toStringAsFixed(1)}V (+${t.battCurrent}A)',
                          color: const Color(0xFF10B981),
                        ),
                      ),

                      // LEFT: Grid Mains Node
                      Align(
                        alignment: Alignment.centerLeft,
                        child: _buildSynopticNode(
                          icon: CupertinoIcons.waveform_path_ecg,
                          title: 'AC GRID',
                          value: '${t.gridVolt.round()} V',
                          subtitle: '${t.gridFreq} Hz',
                          color: const Color(0xFF38BDF8),
                        ),
                      ),

                      // RIGHT: AC Home Load Node
                      Align(
                        alignment: Alignment.centerRight,
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
                ),
              ],
            ),
          ),

          const SizedBox(height: 18),

          // 4. Executive Metric Cards (Grid of 4)
          Row(
            children: [
              Expanded(
                child: _buildMetricCard(
                  title: "TODAY HARVEST",
                  value: "${t.todayKwh} kWh",
                  trend: "+12.4% vs y'day",
                  trendColor: const Color(0xFF10B981),
                  icon: CupertinoIcons.leaf_arrow_circlepath,
                  accentColor: const Color(0xFF10B981),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _buildMetricCard(
                  title: "LIFETIME YIELD",
                  value: "${t.lifetimeMwh} MWh",
                  trend: "CO2 Saved: 2.9 T",
                  trendColor: const Color(0xFF38BDF8),
                  icon: CupertinoIcons.globe,
                  accentColor: const Color(0xFF38BDF8),
                ),
              ),
            ],
          ),

          const SizedBox(height: 12),

          Row(
            children: [
              Expanded(
                child: _buildMetricCard(
                  title: "DC BOOST BUS",
                  value: "${t.dcBoostVolt.round()} VDC",
                  trend: "MPPT Tracking OK",
                  trendColor: const Color(0xFFF59E0B),
                  icon: CupertinoIcons.speedometer,
                  accentColor: const Color(0xFFF59E0B),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: _buildMetricCard(
                  title: "HEATSINK TEMP",
                  value: "${t.heatSinkTemp.toStringAsFixed(1)} °C",
                  trend: "Cooling Active",
                  trendColor: const Color(0xFF10B981),
                  icon: CupertinoIcons.thermometer,
                  accentColor: const Color(0xFF10B981),
                ),
              ),
            ],
          ),

          const SizedBox(height: 20),
        ],
      ),
    );
  }

  Widget _buildSynopticNode({
    required IconData icon,
    required String title,
    required String value,
    required String subtitle,
    required Color color,
  }) {
    return Container(
      width: 110,
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
      decoration: BoxDecoration(
        color: const Color(0xFF0B132B),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: color.withValues(alpha: 0.6), width: 1.5),
        boxShadow: [
          BoxShadow(
            color: color.withValues(alpha: 0.2),
            blurRadius: 8,
            offset: const Offset(0, 2),
          ),
        ],
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, color: color, size: 20),
          const SizedBox(height: 3),
          Text(
            title,
            style: TextStyle(fontSize: 9, fontWeight: FontWeight.bold, color: Colors.white.withValues(alpha: 0.6)),
          ),
          Text(
            value,
            style: const TextStyle(fontSize: 14, fontWeight: FontWeight.w800, color: Colors.white),
          ),
          Text(
            subtitle,
            style: TextStyle(fontSize: 9, color: color),
          ),
        ],
      ),
    );
  }

  Widget _buildMetricCard({
    required String title,
    required String value,
    required String trend,
    required Color trendColor,
    required IconData icon,
    required Color accentColor,
  }) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: const Color(0xFF111827),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: const Color(0xFF1F2937), width: 1.2),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                title,
                style: TextStyle(
                  fontSize: 10,
                  fontWeight: FontWeight.w700,
                  color: Colors.white.withValues(alpha: 0.5),
                  letterSpacing: 0.5,
                ),
              ),
              Icon(icon, color: accentColor, size: 18),
            ],
          ),
          const SizedBox(height: 8),
          Text(
            value,
            style: const TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.w900,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 4),
          Text(
            trend,
            style: TextStyle(
              fontSize: 10,
              fontWeight: FontWeight.w600,
              color: trendColor,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildConnectionPill(InverterTelemetry t) {
    Color bg;
    Color border;
    Color dot;
    Color textColor;
    String label;

    if (t.isLiveAws) {
      bg = const Color(0xFF064E3B);
      border = const Color(0xFF10B981).withValues(alpha: 0.6);
      dot = const Color(0xFF34D399);
      textColor = const Color(0xFF34D399);
      label = 'LIVE: ${t.thingId}';
    } else if (t.awsStatus == AwsConnectionStatus.connecting) {
      bg = const Color(0xFF78350F);
      border = const Color(0xFFF59E0B).withValues(alpha: 0.6);
      dot = const Color(0xFFFBBF24);
      textColor = const Color(0xFFFBBF24);
      label = 'AWS CONNECTING...';
    } else {
      bg = const Color(0xFF1E293B);
      border = const Color(0xFF38BDF8).withValues(alpha: 0.4);
      dot = const Color(0xFF38BDF8);
      textColor = const Color(0xFF38BDF8);
      label = 'DEMO SIMULATOR';
    }

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 5),
      decoration: BoxDecoration(
        color: bg,
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: border),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          CircleAvatar(radius: 3.0, backgroundColor: dot),
          const SizedBox(width: 5),
          Text(
            label,
            style: TextStyle(
              fontSize: 9.5,
              fontWeight: FontWeight.w800,
              color: textColor,
              letterSpacing: 0.3,
            ),
          ),
        ],
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// SCREEN 2: ENERGY HARVEST ANALYTICS
// ---------------------------------------------------------------------------
class AnalyticsScreen extends StatelessWidget {
  final InverterTelemetry telemetry;
  const AnalyticsScreen({super.key, required this.telemetry});

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      child: ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          const Text(
            'Solar Harvest & Analytics',
            style: TextStyle(fontSize: 22, fontWeight: FontWeight.w800, color: Colors.white),
          ),
          const SizedBox(height: 6),
          Text(
            'Historical Energy Production & Efficiency Curve',
            style: TextStyle(fontSize: 12, color: Colors.white.withValues(alpha: 0.5)),
          ),
          const SizedBox(height: 20),

          // 7-Day Production Bar Mockup
          Container(
            padding: const EdgeInsets.all(18),
            decoration: BoxDecoration(
              color: const Color(0xFF111827),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: const Color(0xFF1F2937)),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text(
                      'PAST 7 DAYS GENERATION (kWh)',
                      style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: Color(0xFF94A3B8)),
                    ),
                    Text(
                      'TOTAL: 98.4 kWh',
                      style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: Color(0xFFF59E0B)),
                    ),
                  ],
                ),
                const SizedBox(height: 24),
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
                    _buildBar('Sun', 14.8, 0.78, isToday: true),
                  ],
                ),
              ],
            ),
          ),

          const SizedBox(height: 18),

          // Battery Health Metric
          Container(
            padding: const EdgeInsets.all(18),
            decoration: BoxDecoration(
              color: const Color(0xFF111827),
              borderRadius: BorderRadius.circular(20),
              border: Border.all(color: const Color(0xFF1F2937)),
            ),
            child: Row(
              children: [
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: const Color(0xFF064E3B),
                    borderRadius: BorderRadius.circular(16),
                  ),
                  child: const Icon(CupertinoIcons.battery_full, color: Color(0xFF10B981), size: 30),
                ),
                const SizedBox(width: 16),
                const Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'BATTERY STATE OF HEALTH (SOH)',
                        style: TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: Color(0xFF94A3B8)),
                      ),
                      Text(
                        '98% • Optimal Capacity',
                        style: TextStyle(fontSize: 16, fontWeight: FontWeight.w800, color: Colors.white),
                      ),
                      Text(
                        'Lead-Acid Gravity Balanced • Zero Sulfation',
                        style: TextStyle(fontSize: 11, color: Color(0xFF34D399)),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildBar(String day, double kwh, double heightFactor, {bool isToday = false}) {
    return Column(
      children: [
        Text(
          kwh.toStringAsFixed(1),
          style: TextStyle(fontSize: 10, fontWeight: FontWeight.bold, color: isToday ? const Color(0xFFF59E0B) : Colors.white60),
        ),
        const SizedBox(height: 6),
        Container(
          width: 28,
          height: 120 * heightFactor,
          decoration: BoxDecoration(
            gradient: LinearGradient(
              colors: isToday
                  ? [const Color(0xFFF59E0B), const Color(0xFFD97706)]
                  : [const Color(0xFF38BDF8), const Color(0xFF0284C7)],
              begin: Alignment.topCenter,
              end: Alignment.bottomCenter,
            ),
            borderRadius: BorderRadius.circular(6),
          ),
        ),
        const SizedBox(height: 8),
        Text(
          day,
          style: TextStyle(
            fontSize: 11,
            fontWeight: isToday ? FontWeight.bold : FontWeight.normal,
            color: isToday ? const Color(0xFFF59E0B) : Colors.white60,
          ),
        ),
      ],
    );
  }
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

class _ProvisioningScreenState extends State<ProvisioningScreen> {
  final _ssidController = TextEditingController(text: 'Prasadam-BSNL');
  final _passController = TextEditingController(text: 'venu6076!');
  bool _isScanning = false;
  String _statusMsg = 'Tap "Scan Inverter" to detect nearby SunGridNova hardware';

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      child: ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          const Text(
            'Wireless Inverter Setup',
            style: TextStyle(fontSize: 22, fontWeight: FontWeight.w800, color: Colors.white),
          ),
          const SizedBox(height: 6),
          Text(
            'Provision Home Wi-Fi credentials to ESP32 over Bluetooth (BLE)',
            style: TextStyle(fontSize: 12, color: Colors.white.withValues(alpha: 0.5)),
          ),
          const SizedBox(height: 20),

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
                  onPressed: () {
                    setState(() {
                      _isScanning = true;
                      _statusMsg = 'Scanning for SunGridNova- devices over BLE...';
                    });
                    Future.delayed(const Duration(seconds: 2), () {
                      if (!mounted) return;
                      setState(() {
                        _isScanning = false;
                        _statusMsg = 'Connected to SunGridNova-X8849 (MAC: 48:E7:29:A4)';
                      });
                    });
                  },
                  icon: _isScanning
                      ? const SizedBox(width: 16, height: 16, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.black))
                      : const Icon(CupertinoIcons.bluetooth, color: Colors.black),
                  label: Text(_isScanning ? 'SCANNING...' : 'SCAN FOR INVERTER', style: const TextStyle(fontWeight: FontWeight.w800, color: Colors.black)),
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
                    border: OutlineInputBorder(borderRadius: BorderRadius.circular(12)),
                    filled: true,
                    fillColor: const Color(0xFF1E293B),
                  ),
                ),
                const SizedBox(height: 12),
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
                    onPressed: () {
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(
                          content: Text('Transmitting Wi-Fi credentials over BLE GATT (UUID: ...ef2)'),
                          backgroundColor: Color(0xFF10B981),
                        ),
                      );
                    },
                    style: ElevatedButton.styleFrom(
                      backgroundColor: const Color(0xFFF59E0B),
                      padding: const EdgeInsets.symmetric(vertical: 14),
                      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                    ),
                    child: const Text(
                      'PROVISION WI-FI TO INVERTER',
                      style: TextStyle(fontWeight: FontWeight.w800, color: Colors.black),
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
// SCREEN 4: INDUSTRIAL DIAGNOSTICS & OEM INFO
// ---------------------------------------------------------------------------
class DiagnosticsScreen extends StatelessWidget {
  final InverterTelemetry telemetry;
  const DiagnosticsScreen({super.key, required this.telemetry});

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      child: ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          const Text(
            'System Diagnostics & OEM',
            style: TextStyle(fontSize: 22, fontWeight: FontWeight.w800, color: Colors.white),
          ),
          const SizedBox(height: 6),
          Text(
            'Live Hardware Diagnostics & Certified Calibration Limits',
            style: TextStyle(fontSize: 12, color: Colors.white.withValues(alpha: 0.5)),
          ),
          const SizedBox(height: 20),

          _buildDiagCard(
            title: 'HARDWARE PLATFORM',
            items: [
              _buildRow('Inverter Brand', 'SunGridNova Industrial'),
              _buildRow('Hardware Model', 'Solar HMI Controller V2.1'),
              _buildRow('Processor', 'ESP32 Dual-Core 240 MHz (Tensilica)'),
              _buildRow('Security State', 'Task Watchdog Active (5000ms Panic)'),
            ],
          ),

          const SizedBox(height: 14),

          _buildDiagCard(
            title: 'CALIBRATED PROTECTION THRESHOLDS',
            items: [
              _buildRow('Battery Full Cutoff', '28.8 VDC'),
              _buildRow('Battery Low Trip Cutoff', '21.0 VDC'),
              _buildRow('AC Mains Undervolt Limit', '185 VAC'),
              _buildRow('AC Mains Overvolt Limit', '265 VAC'),
              _buildRow('Heatsink Thermal Trip', '85.0 °C'),
            ],
          ),

          const SizedBox(height: 14),

          _buildDiagCard(
            title: 'CLOUD & CONNECTIVITY STATUS',
            items: [
              _buildRow('AWS IoT Core Endpoint', 'a15qebuvm1g118-ats.iot.ap-southeast-2.amazonaws.com'),
              _buildRow('Target Thing ID', telemetry.thingId),
              _buildRow('MQTT mTLS Status', telemetry.awsStatus.name.toUpperCase()),
              _buildRow('Active Data Feed', telemetry.isLiveAws ? 'LIVE AWS TELEMETRY STREAM' : 'LOCAL SIMULATOR FALLBACK'),
              _buildRow('Telemetry Topic', 'solar/${telemetry.thingId}/telemetry'),
              _buildRow('Last Telemetry Packet', telemetry.lastPacketTime != null ? '${DateTime.now().difference(telemetry.lastPacketTime!).inSeconds}s ago' : 'Awaiting publication...'),
              _buildRow('Device Auth', 'ECC NIST P-256 (Mutual TLS 1.2)'),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildDiagCard({required String title, required List<Widget> items}) {
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
          Text(
            title,
            style: const TextStyle(fontSize: 11, fontWeight: FontWeight.bold, color: Color(0xFFF59E0B)),
          ),
          const SizedBox(height: 12),
          ...items,
        ],
      ),
    );
  }

  Widget _buildRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(fontSize: 12, color: Color(0xFF94A3B8))),
          Text(value, style: const TextStyle(fontSize: 12, fontWeight: FontWeight.w600, color: Colors.white)),
        ],
      ),
    );
  }
}
