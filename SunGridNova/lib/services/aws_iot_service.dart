import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

enum AwsConnectionStatus {
  disconnected,
  connecting,
  connected,
  failed,
}

class AwsIotService {
  static final AwsIotService _instance = AwsIotService._internal();
  factory AwsIotService() => _instance;
  AwsIotService._internal();

  static const String endpoint = 'a15qebuvm1g118-ats.iot.ap-southeast-2.amazonaws.com';
  static const int port = 8883;

  static const String rootCa = '''-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----''';

  static const String clientCert = '''-----BEGIN CERTIFICATE-----
MIICFjCCAbygAwIBAgIUU+zmCtwJr5LiZE4Ock9YzJulpQkwCgYIKoZIzj0EAwIw
cTELMAkGA1UEBhMCSU4xDzANBgNVBAgMBktlcmFsYTEPMA0GA1UEBwwGS2FubnVy
MRQwEgYDVQQKDAtTdW5HcmlkTm92YTEMMAoGA1UECwwDSW9UMRwwGgYDVQQDDBNT
dW5HcmlkTm92YSBSb290IENBMCAXDTI1MDcwMjE4MjgwM1oYDzIxMjQwNjA4MTgy
ODAzWjBfMQswCQYDVQQGEwJJTjEPMA0GA1UECAwGS2VyYWxhMQ8wDQYDVQQHDAZL
YW5udXIxFDASBgNVBAoMC1N1bkdyaWROb3ZhMQwwCgYDVQQLDANJb1QxCjAIBgNV
BAMMATEwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS4O/fxZ3FQSAJU5vUPmhJ0
FPpRjDaZ8cBYurYGPjzPVTOIKaswRdKwgpaw0P6fZtoy41E/hiyJ0a1BE4Mr0gFv
o0IwQDAdBgNVHQ4EFgQUpenLX3F72HXstrVVRZBKQnvWOxwwHwYDVR0jBBgwFoAU
rrrDrznIbmEJXH0Ev4t2qZqpi78wCgYIKoZIzj0EAwIDSAAwRQIhAJrwVfW/Apin
NZ5pmfR70NYGE+L+X0QutHnSQQRE/SCHAiAqL/yw8SL3steM5S6tmfp1PmE4pZOV
HFInMT64gm7ukw==
-----END CERTIFICATE-----''';

  static const String clientKey = '''-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIAeDoK8SV4NakJNmBp5XZtvVZlqVoTPNUqC4EKAk6qteoAoGCCqGSM49
AwEHoUQDQgAEuDv38WdxUEgCVOb1D5oSdBT6UYw2mfHAWLq2Bj48z1UziCmrMEXS
sIKWsND+n2baMuNRP4YsidGtQRODK9IBbw==
-----END EC PRIVATE KEY-----''';

  MqttServerClient? _client;
  AwsConnectionStatus _status = AwsConnectionStatus.disconnected;
  final _statusController = StreamController<AwsConnectionStatus>.broadcast();
  final _telemetryController = StreamController<Map<String, dynamic>>.broadcast();
  final _alertController = StreamController<Map<String, dynamic>>.broadcast();
  Timer? _presenceTimer;
  Timer? _retryTimer;

  Stream<AwsConnectionStatus> get statusStream => _statusController.stream;
  Stream<Map<String, dynamic>> get telemetryStream => _telemetryController.stream;
  Stream<Map<String, dynamic>> get alertStream => _alertController.stream;
  AwsConnectionStatus get status => _status;

  String? _lastReceivedThing;
  String? get lastReceivedThing => _lastReceivedThing;

  void _scheduleRetry() {
    _retryTimer?.cancel();
    _retryTimer = Timer(const Duration(seconds: 4), () {
      if (_status != AwsConnectionStatus.connected && _status != AwsConnectionStatus.connecting) {
        debugPrint('🔄 [AwsIotService] Retrying connection to AWS IoT Core...');
        connect();
      }
    });
  }

  Future<void> connect() async {
    if (_status == AwsConnectionStatus.connecting || _status == AwsConnectionStatus.connected) {
      return;
    }

    _setStatus(AwsConnectionStatus.connecting);

    try {
      final randSuffix = (Random().nextInt(9000) + 1000).toString();
      final clientId = 'SunGridNova-App-$randSuffix';

      final client = MqttServerClient.withPort(endpoint, clientId, port);
      _client = client;

      client.secure = true;
      client.logging(on: false);
      client.keepAlivePeriod = 30;
      client.autoReconnect = true;
      client.resubscribeOnAutoReconnect = true;

      // Industrial Grade TLS 1.2 with Mutual Authentication (mTLS)
      final context = SecurityContext(withTrustedRoots: false);
      context.setTrustedCertificatesBytes(utf8.encode(rootCa));
      context.useCertificateChainBytes(utf8.encode(clientCert));
      context.usePrivateKeyBytes(utf8.encode(clientKey));
      client.securityContext = context;

      client.onConnected = () {
        debugPrint('✅ [AwsIotService] Connected to AWS IoT Core MQTT broker');
        _retryTimer?.cancel();
        _retryTimer = null;
        _setStatus(AwsConnectionStatus.connected);
        _startPresenceHeartbeat();

        // Subscribe to all SunGridNova devices telemetry & alerts
        client.subscribe('solar/+/telemetry', MqttQos.atLeastOnce);
        client.subscribe('solar/#', MqttQos.atLeastOnce);
        client.subscribe('solar/+/alerts', MqttQos.atLeastOnce);
      };

      client.onDisconnected = () {
        debugPrint('⚠️ [AwsIotService] Disconnected from Server');
        _stopPresenceHeartbeat();
        _setStatus(AwsConnectionStatus.disconnected);
        _scheduleRetry();
      };

      client.onAutoReconnect = () {
        debugPrint('🔄 [AwsIotService] Auto-reconnecting to Server...');
        _stopPresenceHeartbeat();
        _setStatus(AwsConnectionStatus.connecting);
      };

      final connMessage = MqttConnectMessage()
          .withClientIdentifier(clientId)
          .startClean();
      client.connectionMessage = connMessage;

      final status = await client.connect();
      if (status?.state != MqttConnectionState.connected) {
        debugPrint('⚠️ [AwsIotService] Connect returned status: ${status?.state}');
        _setStatus(AwsConnectionStatus.failed);
        _scheduleRetry();
      }

      // Listen for incoming messages
      client.updates?.listen((List<MqttReceivedMessage<MqttMessage>> messages) {
        for (final msg in messages) {
          final recMessage = msg.payload as MqttPublishMessage;
          final payloadStr = MqttPublishPayload.bytesToStringAsString(recMessage.payload.message);
          final topic = msg.topic;
          debugPrint('📥 [Server Sync] Received on topic $topic: $payloadStr');

          try {
            final data = jsonDecode(payloadStr) as Map<String, dynamic>;
            if (topic.endsWith('/telemetry') || topic.contains('telemetry')) {
              _lastReceivedThing = data['thing']?.toString() ?? 'SunGridNova-49F8';
              _telemetryController.add(data);
            } else if (topic.endsWith('/alerts')) {
              _alertController.add(data);
            }
          } catch (e) {
            debugPrint('Error parsing MQTT payload: $e');
          }
        }
      });
    } catch (e) {
      debugPrint('❌ [AwsIotService] Connection failed: $e');
      _setStatus(AwsConnectionStatus.failed);
      _scheduleRetry();
    }
  }

  void _publishPresence() {
    if (_client == null || _status != AwsConnectionStatus.connected) return;
    try {
      final builder = MqttClientPayloadBuilder();
      builder.addString(jsonEncode({'active': 1, 'ts': DateTime.now().millisecondsSinceEpoch}));
      _client!.publishMessage('solar/presence', MqttQos.atLeastOnce, builder.payload!);
      if (_lastReceivedThing != null) {
        _client!.publishMessage('solar/$_lastReceivedThing/presence', MqttQos.atLeastOnce, builder.payload!);
      }
      debugPrint('📡 [AwsIotService] Sent presence heartbeat to inverter');
    } catch (e) {
      debugPrint('Error publishing presence: $e');
    }
  }

  void _startPresenceHeartbeat() {
    _stopPresenceHeartbeat();
    _publishPresence();
    _presenceTimer = Timer.periodic(const Duration(seconds: 10), (_) {
      _publishPresence();
    });
  }

  void _stopPresenceHeartbeat() {
    _presenceTimer?.cancel();
    _presenceTimer = null;
  }

  void disconnect() {
    _retryTimer?.cancel();
    _retryTimer = null;
    _stopPresenceHeartbeat();
    _client?.disconnect();
    _setStatus(AwsConnectionStatus.disconnected);
  }

  void _setStatus(AwsConnectionStatus newStatus) {
    _status = newStatus;
    _statusController.add(newStatus);
  }

  void dispose() {
    _retryTimer?.cancel();
    _retryTimer = null;
    _stopPresenceHeartbeat();
    _client?.disconnect();
    _statusController.close();
    _telemetryController.close();
    _alertController.close();
  }
}
