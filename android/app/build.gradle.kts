// B-Klasa Hero — moduł aplikacji.
// Specyfika:
//   * minSdk = 24 (Android 7.0) — pokrywa >95% urządzeń w PL,
//   * ABI: arm64-v8a (priorytet) + armeabi-v7a (legacy) — bez x86_64 (nie chcemy
//     rozmiaru), betoniarki F-Droid budują per-ABI,
//   * kompresja zasobów gzip/resources — bez tego CSV miejscowości puchnie >1 MB,
//   * Compose UI z Material 3 — typografia inspirowana gazetkami okręgowymi,
//   * Room (KSP) do trwałych save'ów, NDK (CMake) do budowy libbkh_bridge.so.
// SPDX-License-Identifier: GPL-3.0-or-later
plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("com.google.devtools.ksp")
}

// Wersja z nadpisaniem przez CI: `-PversionName=... -PversionCode=...`.
// Domyślnie używamy stałej semver z rdzenia (patrz scripts/bump-version.sh).
val versionNameOverride = (project.findProperty("versionName") as String?) ?: "0.1.0"
val versionCodeOverride = (project.findProperty("versionCode") as String?)?.toIntOrNull() ?: 1

android {
    namespace = "pl.bklasahero"
    compileSdk = 35
    buildToolsVersion = "35.0.0"
    ndkVersion = "27.2.12479018"

    defaultConfig {
        applicationId = "pl.bklasahero"
        minSdk = 24
        targetSdk = 35
        versionCode = versionCodeOverride
        versionName = versionNameOverride
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_static",
                    "-DCMAKE_BUILD_TYPE=Release",
                )
                // Offline (F-Droid): -Pbkh.deps.dir=$$srclib$$ → rdzeń używa
                // lokalnych źródeł json/googletest zamiast FetchContent.
                (project.findProperty("bkh.deps.dir") as String?)?.let { dir ->
                    arguments += "-DBKH_DEPS_DIR=$dir"
                }
                cppFlags += "-std=c++23"
                cFlags += listOf("-fvisibility=hidden")
            }
        }

        // Jeden pakiet per lokalizacja (F-Droid preferuje PL).
        resourceConfigurations += listOf("pl", "en")
    }

    externalNativeBuild {
        cmake {
            path = file("../../bridge/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    // Release jest celowo NIEPODPISANY: F-Droid podpisuje własnym kluczem.
    // Podpis dla GitHub Releases wykonuje scripts/sign-apk.sh na APK po buildzie
    // (klucze żyją w sekretach CI, nie w repo).
    buildTypes {
        getByName("debug") {
            isMinifyEnabled = false
            isDebuggable = true
        }
        getByName("release") {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
        freeCompilerArgs += listOf(
            "-Xjvm-default=all",
        )
    }

    buildFeatures {
        compose = true
        buildConfig = true
    }

    composeOptions {
        // Wersja kompilatora Compose dla Kotlin 1.9.24 (tablica kompatybilności
        // na https://developer.android.com/jetpack/androidx/releases/compose-kotlin).
        kotlinCompilerExtensionVersion = "1.5.14"
    }

    packaging {
        resources {
            excludes += setOf(
                "/META-INF/{AL2.0,LGPL2.1}",
                "META-INF/DEPENDENCIES",
                "META-INF/LICENSE*",
                "META-INF/NOTICE*",
            )
        }
    }
}

dependencies {
    val composeBom = platform("androidx.compose:compose-bom:2024.06.00")
    implementation(composeBom)
    androidTestImplementation(composeBom)

    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.activity:activity-compose:1.9.0")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.2")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.8.2")

    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")

    implementation("androidx.room:room-runtime:2.6.1")
    implementation("androidx.room:room-ktx:2.6.1")
    ksp("androidx.room:room-compiler:2.6.1")

    implementation("androidx.navigation:navigation-compose:2.7.7")

    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.8.1")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.3")

    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")

    testImplementation("junit:junit:4.13.2")
    testImplementation("org.jetbrains.kotlinx:kotlinx-coroutines-test:1.8.1")
    androidTestImplementation("androidx.test.ext:junit:1.2.1")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.6.1")
    androidTestImplementation("androidx.compose.ui:ui-test-junit4")
}