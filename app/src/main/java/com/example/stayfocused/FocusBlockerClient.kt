package com.example.stayfocused

import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebView
import android.webkit.WebViewClient
import java.io.ByteArrayInputStream

class FocusBlockerClient : WebViewClient() {

    // যেসব ওয়েবসাইট বা কি-ওয়ার্ড ব্লক করতে চান তার তালিকা
    private val blockedSites = listOf(
        "facebook.com",
        "instagram.com",
        "tiktok.com",
        "doubleclick.net", // Ad server
        "googleadservices.com" // Ad server
    )

    override fun shouldInterceptRequest(
        view: WebView?,
        request: WebResourceRequest?
    ): WebResourceResponse? {
        val url = request?.url.toString()

        // URL টি আমাদের ব্লক লিস্টে আছে কিনা চেক করা হচ্ছে
        for (site in blockedSites) {
            if (url.contains(site, ignoreCase = true)) {
                // ব্লক করা সাইট হলে একটি ফাঁকা রেসপন্স অথবা সতর্কবার্তা দেখাবে
                val blockMessage = "<html><body><h2 style='color:red; text-align:center; margin-top:50px;'>এই সাইটটি ফোকাস ধরে রাখার জন্য ব্লক করা হয়েছে!</h2></body></html>"
                val inputStream = ByteArrayInputStream(blockMessage.toByteArray())
                
                return WebResourceResponse(
                    "text/html",
                    "UTF-8",
                    inputStream
                )
            }
        }
        
        // ব্লক লিস্টে না থাকলে স্বাভাবিকভাবে সাইট লোড হবে
        return super.shouldInterceptRequest(view, request)
    }

    // অ্যাপের ভেতরেই যেন লিংক ওপেন হয়, অন্য ব্রাউজারে না যায়
    override fun shouldOverrideUrlLoading(view: WebView?, request: WebResourceRequest?): Boolean {
        view?.loadUrl(request?.url.toString())
        return true
    }
}
