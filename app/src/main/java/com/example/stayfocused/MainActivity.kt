package com.example.stayfocused

import android.annotation.SuppressLint
import android.os.Bundle
import android.webkit.WebView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    private lateinit var webView: WebView

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        webView = findViewById(R.id.mainWebView)

        // WebView এর জন্য কিছু প্রয়োজনীয় সেটিংস
        webView.settings.apply {
            javaScriptEnabled = true // জাভাস্ক্রিপ্ট অন করা হলো
            domStorageEnabled = true // ওয়েব সাইটের লোকাল স্টোরেজ কাজের জন্য
        }

        // আমাদের তৈরি করা ব্লকার লজিক যুক্ত করা হলো
        webView.webViewClient = FocusBlockerClient()

        // ডিফল্ট হিসেবে একটি কাজের বা পড়াশোনার ওয়েবসাইট লোড করুন
        webView.loadUrl("https://www.google.com") 
    }

    // ব্যাক বাটন চাপলে যেন অ্যাপ থেকে বের না হয়ে আগের পেইজে যায়
    override fun onBackPressed() {
        if (webView.canGoBack()) {
            webView.goBack()
        } else {
            super.onBackPressed()
        }
    }
}
