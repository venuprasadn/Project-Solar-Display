package com.sungridnova.sungridnova

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.location.LocationManager
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.net.wifi.WifiInfo
import android.net.wifi.WifiManager
import android.os.Build
import android.provider.Settings
import android.util.Log
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {
    private val CHANNEL = "com.sungridnova/wifi"
    private var pendingResult: MethodChannel.Result? = null
    private var currentSsid: String? = null
    private var isWifiConnected: Boolean = false
    private var networkCallback: ConnectivityManager.NetworkCallback? = null

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)

        registerWifiCallback()

        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, CHANNEL).setMethodCallHandler { call, result ->
            when (call.method) {
                "getConnectedWifiInfo" -> {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        val fineGranted = checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED
                        val coarseGranted = checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED
                        if (!fineGranted && !coarseGranted) {
                            pendingResult = result
                            requestPermissions(
                                arrayOf(
                                    Manifest.permission.ACCESS_FINE_LOCATION,
                                    Manifest.permission.ACCESS_COARSE_LOCATION
                                ),
                                1001
                            )
                            return@setMethodCallHandler
                        }
                    }
                    val info = getConnectedWifiInfo()
                    result.success(info)
                }
                "openLocationSettings" -> {
                    try {
                        val intent = Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS)
                        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK
                        startActivity(intent)
                        result.success(true)
                    } catch (e: Exception) {
                        result.error("ERR", e.message, null)
                    }
                }
                else -> result.notImplemented()
            }
        }
    }

    private fun registerWifiCallback() {
        try {
            val connManager = getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager ?: return
            val request = NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                .build()

            // Unregister prior callback if registered
            if (networkCallback != null) {
                try {
                    connManager.unregisterNetworkCallback(networkCallback!!)
                } catch (_: Exception) {}
                networkCallback = null
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                networkCallback = object : ConnectivityManager.NetworkCallback(FLAG_INCLUDE_LOCATION_INFO) {
                    override fun onCapabilitiesChanged(network: Network, capabilities: NetworkCapabilities) {
                        isWifiConnected = true
                        val wifiInfo = capabilities.transportInfo as? WifiInfo
                        extractSsid(wifiInfo?.ssid)
                    }

                    override fun onLost(network: Network) {
                        isWifiConnected = false
                        currentSsid = null
                    }
                }
                connManager.registerNetworkCallback(request, networkCallback!!)
            } else {
                networkCallback = object : ConnectivityManager.NetworkCallback() {
                    override fun onCapabilitiesChanged(network: Network, capabilities: NetworkCapabilities) {
                        isWifiConnected = true
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                            val wifiInfo = capabilities.transportInfo as? WifiInfo
                            extractSsid(wifiInfo?.ssid)
                        }
                    }

                    override fun onLost(network: Network) {
                        isWifiConnected = false
                        currentSsid = null
                    }
                }
                connManager.registerNetworkCallback(request, networkCallback!!)
            }
        } catch (e: Exception) {
            Log.e("SunGridWifi", "Error registering network callback: $e")
        }
    }

    private fun extractSsid(rawSsid: String?) {
        if (rawSsid == null) return
        var s = rawSsid
        if (s.startsWith("\"") && s.endsWith("\"") && s.length >= 2) {
            s = s.substring(1, s.length - 1)
        }
        if (s != "<unknown ssid>" && s.isNotEmpty()) {
            currentSsid = s
            Log.d("SunGridWifi", "Extracted connected SSID: $s")
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1001) {
            registerWifiCallback()
            val info = getConnectedWifiInfo()
            pendingResult?.success(info)
            pendingResult = null
        }
    }

    private fun getConnectedWifiInfo(): Map<String, Any?> {
        val res = mutableMapOf<String, Any?>()
        val connManager = getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
        val network = connManager?.activeNetwork

        val capabilities = if (network != null) connManager.getNetworkCapabilities(network) else null
        val hasWifi = capabilities?.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) == true || isWifiConnected

        val locManager = getSystemService(Context.LOCATION_SERVICE) as? LocationManager
        val isLocationEnabled = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            locManager?.isLocationEnabled == true
        } else {
            true
        }
        res["isLocationEnabled"] = isLocationEnabled

        if (!hasWifi) {
            res["isConnected"] = false
            res["ssid"] = null
            return res
        }

        res["isConnected"] = true

        // 1. Try cached SSID from FLAG_INCLUDE_LOCATION_INFO callback
        if (currentSsid != null && currentSsid!!.isNotEmpty() && currentSsid != "<unknown ssid>") {
            res["ssid"] = currentSsid
            return res
        }

        // 2. Try capabilities.transportInfo directly
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && capabilities != null) {
            val wifiInfo = capabilities.transportInfo as? WifiInfo
            extractSsid(wifiInfo?.ssid)
        }

        // 3. Fallback to WifiManager
        if (currentSsid == null || currentSsid == "<unknown ssid>") {
            val wifiManager = applicationContext.getSystemService(Context.WIFI_SERVICE) as? WifiManager
            val wifiInfo = wifiManager?.connectionInfo
            extractSsid(wifiInfo?.ssid)
        }

        res["ssid"] = currentSsid
        return res
    }

    override fun onDestroy() {
        super.onDestroy()
        try {
            if (networkCallback != null) {
                val connManager = getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
                connManager?.unregisterNetworkCallback(networkCallback!!)
            }
        } catch (_: Exception) {}
    }
}
