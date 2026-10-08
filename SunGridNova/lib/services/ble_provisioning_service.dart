import 'dart:async';
import 'dart:convert';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class BleProvisioningService {
  static final BleProvisioningService _instance = BleProvisioningService._internal();
  factory BleProvisioningService() => _instance;
  BleProvisioningService._internal();

  // Standard SunGridNova Provisioning BLE UUIDs
  static final Guid serviceUuid = Guid("12345678-1234-5678-1234-56789abcdef0");
  static final Guid txCharUuid  = Guid("12345678-1234-5678-1234-56789abcdef1"); // Read / Notify
  static final Guid rxCharUuid  = Guid("12345678-1234-5678-1234-56789abcdef2"); // Write

  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _txChar;
  BluetoothCharacteristic? _rxChar;

  StreamSubscription? _scanSub;
  StreamSubscription? _txNotifySub;

  final _statusMessageController = StreamController<String>.broadcast();
  Stream<String> get statusMessageStream => _statusMessageController.stream;

  final _deviceFoundController = StreamController<BluetoothDevice>.broadcast();
  Stream<BluetoothDevice> get deviceFoundStream => _deviceFoundController.stream;

  final _isProvisionedController = StreamController<bool>.broadcast();
  Stream<bool> get isProvisionedStream => _isProvisionedController.stream;

  BluetoothDevice? get connectedDevice => _connectedDevice;
  bool get isConnected => _connectedDevice != null && _rxChar != null;

  String? _deviceMac;
  String? get deviceMac => _deviceMac;

  String? _inverterSsid;
  String? get inverterSsid => _inverterSsid;

  String? _inverterIp;
  String? get inverterIp => _inverterIp;

  int? _inverterRssi;
  int? get inverterRssi => _inverterRssi;

  String? _vendorName;
  String? get vendorName => _vendorName;

  Future<void> startScan() async {
    _statusMessageController.add('Scanning for nearby SunGridNova inverters...');

    // Stop any existing scan
    await FlutterBluePlus.stopScan();

    _scanSub?.cancel();
    _scanSub = FlutterBluePlus.scanResults.listen((results) {
      for (final r in results) {
        final advName = r.advertisementData.advName;
        final platName = r.device.platformName;
        final name = advName.isNotEmpty ? advName : platName;
        if (name.startsWith('SunGridNova')) {
          _deviceFoundController.add(r.device);
        }
      }
    });

    try {
      await FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 15),
      );
    } catch (e) {
      _statusMessageController.add('Scan error: $e');
    }
  }

  Future<void> stopScan() async {
    await FlutterBluePlus.stopScan();
    _scanSub?.cancel();
  }

  Future<bool> connect(BluetoothDevice device) async {
    _statusMessageController.add('Connecting to ${device.advName}...');
    try {
      await device.connect(timeout: const Duration(seconds: 10), autoConnect: false);
      _connectedDevice = device;

      _statusMessageController.add('Discovering BLE services...');
      final services = await device.discoverServices();

      for (final s in services) {
        if (s.uuid == serviceUuid) {
          for (final c in s.characteristics) {
            if (c.uuid == txCharUuid) {
              _txChar = c;
            } else if (c.uuid == rxCharUuid) {
              _rxChar = c;
            }
          }
        }
      }

      if (_txChar != null && _rxChar != null) {
        _statusMessageController.add('Connected to ${_connectedDevice?.advName}. Reading state...');

        // Subscribe to status updates from inverter
        await _txChar!.setNotifyValue(true);
        _txNotifySub?.cancel();
        _txNotifySub = _txChar!.onValueReceived.listen(_handleTxData);

        // Initial read
        final initVal = await _txChar!.read();
        _handleTxData(initVal);

        return true;
      } else {
        _statusMessageController.add('Error: Inverter provisioning service not found.');
        await device.disconnect();
        _connectedDevice = null;
        return false;
      }
    } catch (e) {
      _statusMessageController.add('Connection failed: $e');
      _connectedDevice = null;
      return false;
    }
  }

  void _handleTxData(List<int> raw) {
    if (raw.isEmpty) return;
    final str = utf8.decode(raw).trim();
    final parts = str.split(',');
    if (parts.isNotEmpty) {
      _deviceMac = parts[0];
      final bool isWifiOk = parts.length > 1 && parts[1] == '1';
      if (parts.length > 2 && parts[2].isNotEmpty) {
        _inverterSsid = parts[2];
      }
      if (parts.length > 3 && parts[3].isNotEmpty) {
        _inverterIp = parts[3];
      }
      if (parts.length > 4 && parts[4].isNotEmpty) {
        _inverterRssi = int.tryParse(parts[4]);
      }
      if (parts.length > 5 && parts[5].isNotEmpty) {
        _vendorName = parts[5].trim();
      }
      _isProvisionedController.add(isWifiOk);
      _statusMessageController.add(
        isWifiOk
            ? '✅ Inverter connected to "${_inverterSsid ?? "Wi-Fi"}"! (IP: $_inverterIp)'
            : '⚠️ Inverter attempting Wi-Fi link to "${_inverterSsid ?? "AP"}"...',
      );
    }
  }

  Future<bool> sendCredentials({required String ssid, required String password}) async {
    if (_rxChar == null || _connectedDevice == null) {
      _statusMessageController.add('Error: No inverter connected over BLE.');
      return false;
    }

    final payload = '$ssid,$password';
    _statusMessageController.add('Transmitting Wi-Fi credentials to inverter...');

    try {
      await _rxChar!.write(utf8.encode(payload), withoutResponse: false);
      _statusMessageController.add('🔐 Credentials sent! Inverter is joining Wi-Fi...');
      return true;
    } catch (e) {
      _statusMessageController.add('Failed to transmit credentials: $e');
      return false;
    }
  }

  Future<void> disconnect() async {
    _txNotifySub?.cancel();
    _scanSub?.cancel();
    await _connectedDevice?.disconnect();
    _connectedDevice = null;
    _txChar = null;
    _rxChar = null;
    _statusMessageController.add('Disconnected from inverter.');
  }

  void dispose() {
    disconnect();
    _statusMessageController.close();
    _deviceFoundController.close();
    _isProvisionedController.close();
  }
}
