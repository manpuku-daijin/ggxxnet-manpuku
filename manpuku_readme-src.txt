【ビルド環境】
OS:     Windows7(x64)
IDE:    Microsoft Visual Studio 2013 for Windows Desktop
Module: zlib-1.2.8
        directx8 sdk (oct2004)

プリプロセッサの定義にMANPUKUを追加
プラットフォームツールセットをVisual Studio 2013 - Windows XP (v120_xp)に設定
ランタイムライブラリをマルチスレッド (/MT)に設定
最低限必要なバージョンの設定削除
安全な例外ハンドラーを含むイメージをいいえ (/SAFESEH:NO)に設定
VS2013アップデート時のビルド警告修正
ビルド後イベントのコマンドライン削除
ビルド最適化処理をサイズ優先に
インクルードディレクトリ
C:\DX81SDK\include
ライブラリディレクトリ
C:\DX81SDK\lib
追加

ggxxnet80.rcの
#include "afxres.h"
を
#include "winres.h"
に

変更点は主に
#ifdef MANPUKU
#else
#endif
or
#ifdef MANPUKU
#endif
or
#ifndef MANPUKU
#endif
間にあります
