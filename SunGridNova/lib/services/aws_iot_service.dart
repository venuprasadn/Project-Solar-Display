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
MIIDWTCCAkGgAwIBAgIUTK+pfy9imwydIJdBy54c+1bxCyQwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MDcxMDE4MzI1
OFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAPPtxdolwBSC49w7avh8
OaCwJtUVmyK+uKnfgiVV/wqXNIBTYMeZE21GGl5s0msVIoCaWUWKp+BMs5D4uQq3
BH3Wp6U3LHW22CbS1V8xVElL8frod5ceYy71BFLaHrtGLqn2DHmmqmiHbOhR41fU
8B7XeHLPJUJ0f3kg3+mboxD8CmnIpJb7OTzFvhSTyn7M2+QiRHQkWVDrXbDf/pqD
AumbTdKxIVSPOhm0owwQGZvBainso7kfg71Oqjaa+u6TaAQauuumCCqU+lFAq6pY
FyDI7kpG9gi8iwPz+/uKwxVNSEo9N38HqHVKK3NzZOoNdzWbgU8mHRKAsunFUvwB
VFkCAwEAAaNgMF4wHwYDVR0jBBgwFoAUDZbBuzdO/6B36spr6OtWlnghzYowHQYD
VR0OBBYEFJs0GXQ/10vIzNQKvZ6iyJICVpQnMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQDgHVFLqgPUlF+XDPayDRDXNyy1
UwEFscWbKfKOcJLKRUJrvVJlol9Uw4Zlhm/wdjjfeSN6lPjWhgeJfA3puQDhjtFG
Vb617I6nD5gmEgVe7/RrdcgkVRhcbzLuMlCj9Ib4xBnQhgB6FxC/uHaDNnsGl5kV
tlm5UtDIkDzvz24qncr71hCX0TfNtRD+ftf0m+QKEE/I3I7Qi5pIS8jSk8YhwGRO
cgloKODQ5QY5XHRPkGeeEDkOcua+LlA6Cm++KWNiI7HM3QBCl77qWwyHLlKXwjck
9s2PUeko7TsZ/M4WdxlV2G18HRQfKWUKI2QV4sy9AujP4tz+8WSeJQkjH6/w
-----END CERTIFICATE-----''';

  static const String clientKey = '''-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEA8+3F2iXAFILj3Dtq+Hw5oLAm1RWbIr64qd+CJVX/Cpc0gFNg
x5kTbUYaXmzSaxUigJpZRYqn4EyzkPi5CrcEfdanpTcsdbbYJtLVXzFUSUvx+uh3
lx5jLvUEUtoeu0YuqfYMeaaqaIds6FHjV9TwHtd4cs8lQnR/eSDf6ZujEPwKacik
lvs5PMW+FJPKfszb5CJEdCRZUOtdsN/+moMC6ZtN0rEhVI86GbSjDBAZm8FqKeyj
uR+DvU6qNpr67pNoBBq666YIKpT6UUCrqlgXIMjuSkb2CLyLA/P7+4rDFU1ISj03
fweodUorc3Nk6g13NZuBTyYdEoCy6cVS/AFUWQIDAQABAoIBAQDZr1ogIgxWwbCD
e+sssf/jrRAanVuDGF1IDBTjKOmgE+xgkQgPWEaEAEnL9qWZtpTB2/zLGMBUZV7i
g3TvYQD7JDMcOC7PJkuj6gdNGoKznrjmR8th61ZsM3CWV92RF0LRnqjnb5soCaNh
eKLAYWGgxH3TR5VixwBzoqjwm1pc7jJ/GJ7T9yHS78HP+8/kjEg4uESEK3oAGaNk
pDFJfTAj4L3aaRLwapOJ/YxzFR80+rrL5N1o6rHdDKCNyNwFotG4FJ6CZnh8rPnl
dmLqNu23sAPQNfLbSrVHJgbM11ge44pqgXIcF73VPth0gtKgbylY5Y3CZ+dAhktW
WVCeJ44BAoGBAPrWuRkc4TLwZdthWBpWFSHBHJsiJf4yxcDgj77om4fUhPZEmgkD
q6UZZ6PjQOlPfNSDWtLOTYDNulKZqSw1v6FcrDl9qmIAYvXL4A04tt7YiUnuj95z
Z1GepLhaknoA7yTiSP5KripHesvkiubYWV10g1PWgMs4KqfKhh9kYeNhAoGBAPjy
puxf/yimTwENXA/S33fK+EwKwqs4iKSEJ0m/BlUTWZ6U8xC+7E0rk7GnypUp6qUC
+NSGZC+rzVNE5+PKj0Sok7Vfbd7TcXu79XTrf4Qv/eZhtoA8GBvBKX1pY3xO7R/M
k2PkY5q2or7Z0kBxTzWDgSWQ/iaCQMVxvnCnxgv5AoGBAIceubC8bNcKxmOJqXLu
Yg2/v9AVcg/fe8UtcmFtXbKqmUErrSoj7wdNixWuah4D8oNrirY56Wfz6mVqXsXw
4hxjFmcVuX13JdewDi4xGdkrHbFUr+0tjz9ZTPP93h+YdzoQJy/MPMuLm6tPnj9B
1cnQ5Jl52AEgbWHbZ6prYYuhAoGAVkzQHvs8WhwlISk/c+DXRRDguIO2bmK/w8Bo
WkFVcaKum7Ho/TIierITli+jo8gPJrr8Bbi8/GWjXS1y8d2jgtqpseNuFCPaoFlN
QwXsg6ebbgULnjK27NAukOI68bnuq+pgYe2ntdeAXYbnQx3Eprl6yMoVwMXArHG8
4hLXLgkCgYEAjMzP4wgi1g28yrnVPwJsvfgWLc1dtg+5PO8g25fPrCKL5N4i1QvY
BXZD/38ysq7gN3lE+9i/36s25H72RRI81bTfcpbUrp+DVB5bT/JUN2rpt2pN4p/r
CPK83FVZkkAbeFkUe2uDMZK8qzVR2wgUsQrPnjubrtS4qtbCkD3/fSw=
-----END RSA PRIVATE KEY-----''';

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

  void publishMessage(String topic, String payload) {
    if (_client == null || _status != AwsConnectionStatus.connected) return;
    try {
      final builder = MqttClientPayloadBuilder();
      builder.addString(payload);
      _client!.publishMessage(topic, MqttQos.atLeastOnce, builder.payload!);
      debugPrint('📤 [AwsIotService] Published to $topic: $payload');
    } catch (e) {
      debugPrint('Error publishing message: $e');
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
