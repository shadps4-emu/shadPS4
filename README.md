<!--
SPDX-FileCopyrightText: 2024 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

<h1 align="center">
  <br>
  <a href="https://shadps4.net/"><img src="https://github.com/shadps4-emu/shadPS4/blob/main/.github/shadps4.png" width="220"></a>
  <br>
  <b>shadPS4</b>
  <br>
</h1>

<h1 align="center">
 <a href="https://discord.gg/MyZRaBngxA">
        <img src="https://img.shields.io/discord/1080089157554155590?color=5865F2&label=shadPS4 Discord&logo=Discord&logoColor=white" width="240">
 <a href="https://github.com/shadps4-emu/shadPS4/releases/latest">
        <img src="https://img.shields.io/github/downloads/shadps4-emu/shadPS4/total.svg" width="140">
 <a href="https://shadps4.net/">
        <img src="https://img.shields.io/badge/shadPS4-website-8A2BE2" width="150">
 <a href="https://x.com/shadps4">
        <img src="https://img.shields.io/badge/-Join%20us-black?logo=X&logoColor=white" width="100">
 <a href="https://github.com/shadps4-emu/shadPS4/stargazers">
        <img src="https://img.shields.io/github/stars/shadps4-emu/shadPS4" width="120">
</h1>

<p align="center">
  <a href="https://shadps4.net/">
  <img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Screenshots/Sonic Mania.png" width="400">
  <img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Screenshots/Bloodborne.png" width="400">
  <img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Screenshots/Undertale.png" width="400">
  <img src="https://github.com/shadps4-emu/shadPS4/blob/main/documents/Screenshots/We are DOOMED.png" width="400">
</p>

# shadPS4

shadPS4, **Windows**, **Linux** ve **macOS** için yazılmış, C++ tabanlı erken aşama bir **PlayStation 4** emülatörüdür.

Herhangi bir sorunla karşılaşırsanız veya sorularınız olursa, [**Hızlı Başlangıç**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/Quickstart/Quickstart.md) kılavuzuna göz atabilirsiniz.

Bir oyunun çalışıp çalışmadığını kontrol etmek için [**shadPS4 Oyun Uyumluluğu**](https://github.com/shadps4-emu/shadps4-game-compatibility) listesine bakabilirsiniz.

shadPS4'ün geliştirilmesini tartışmak veya fikir önermek için [**Discord sunucumuza**](https://discord.gg/MyZRaBngxA) katılabilirsiniz.

En son haberler için [**X (twitter)**](https://x.com/shadps4) hesabımıza veya [**web sitemize**](https://shadps4.net/) göz atabilirsiniz.

# Durum

Geliştirme aşamasında, [**Sonic Mania**](https://www.youtube.com/watch?v=AAHoNzhHyCU), [**Undertale**](https://youtu.be/5zIvdy65Ro4), [**Dysmantle**](https://youtu.be/b9xzhLBdESE) ve diğer küçük oyunlar çalışmaktadır...

# Neden

Proje, eğlence amaçlı bir proje olarak başladı. Sınırlı boş zaman nedeniyle, shadPS4'ün düzgün bir şey çalıştırabilmesi muhtemelen biraz zaman alacak, ancak küçük ve düzenli güncellemeler yapmaya çalışıyoruz.

# Derleme

## Windows

[**Windows**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/building-windows.md) için derleme talimatlarını kontrol edin.

## Linux

[**Linux**](https://github.com/shadps4-emu/shadPS4/blob/main/documents/building-linux.md) için derleme talimatlarını kontrol edin.

## Derleme durumu

<details>
<summary><b>Windows</b></summary>

| Windows | Build status |
|--------|--------|
|Windows SDL Build|[![Windows-sdl](https://github.com/shadps4-emu/shadPS4/actions/workflows/windows.yml/badge.svg)](https://github.com/shadps4-emu/shadPS4/actions/workflows/windows.yml)
|Windows Qt Build|[![Windows-qt](https://github.com/shadps4-emu/shadPS4/actions/workflows/windows-qt.yml/badge.svg)](https://github.com/shadps4-emu/shadPS4/actions/workflows/windows-qt.yml)
</details>

<details>
<summary><b>Linux</b></summary>

| Linux | Build status |
|--------|--------|
|Linux SDL Build|[![Linux-sdl](https://github.com/shadps4-emu/shadPS4/actions/workflows/linux.yml/badge.svg)](https://github.com/shadps4-emu/shadPS4/actions/workflows/linux.yml)
|Linux Qt Build|[![Linux-qt](https://github.com/shadps4-emu/shadPS4/actions/workflows/linux-qt.yml/badge.svg)](https://github.com/shadps4-emu/shadPS4/actions/workflows/linux-qt.yml)
</details>

<details>
<summary><b>macOS</b></summary>

| macOS | Build status |
|--------|--------|
|macOS SDL Build|[![macOS-sdl](https://github.com/shadps4-emu/shadPS4/actions/workflows/macos.yml/badge.svg)](https://github.com/shadps4-emu/shadPS4/actions/workflows/macos.yml)
|macOS Qt Build|[![macOS-qt](https://github.com/shadps4-emu/shadPS4/actions/workflows/macos-qt.yml/badge.svg)](https://github.com/shadps4-emu/shadPS4/actions/workflows/macos-qt.yml)
</details>

# Klavye Haritası

| Controller button | Keyboard |
|-------------|-------------|
LEFT AXIS UP | W |
LEFT AXIS DOWN | S |
LEFT AXIS LEFT | A |
LEFT AXIS RIGHT | D |
RIGHT AXIS UP | I |
RIGHT AXIS DOWN | K |
RIGHT AXIS LEFT | J |
RIGHT AXIS RIGHT | L |
TRIANGLE | Numpad 8 |
CIRCLE | Numpad 6 |
CROSS | Numpad 2 |
SQUARE | Numpad 4 |
PAD UP | UP |
PAD DOWN | DOWN |
PAD LEFT | LEFT |
PAD RIGHT | RIGHT |
OPTIONS | RETURN |
TOUCH PAD | SPACE |
L1 | Q |
R1 | U |
L2 | E |
R2 | O |
L3 | X |
R3 | M |

# Temel ekip

- [**georgemoralis**](https://github.com/georgemoralis)
- [**raphaelthegreat**](https://github.com/raphaelthegreat)
- [**psucien**](https://github.com/psucien)
- [**skmp**](https://github.com/skmp)
- [**wheremyfoodat**](https://github.com/wheremyfoodat)
- [**raziel1000**](https://github.com/raziel1000)

Logo [**Xphalnos**](https://github.com/Xphalnos) tarafından yapılmıştır.

# Katkıda bulunma

Katkıda bulunmak istiyorsanız, lütfen [**CONTRIBUTING.md**](https://github.com/shadps4-emu/shadPS4/blob/main/CONTRIBUTING.md) dosyasına göz atın.

Bir PR açın, biz de kontrol edelim :)

# Katkıda bulunanlar

<a href="https://github.com/shadps4-emu/shadPS4/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=shadps4-emu/shadPS4&max=15">
</a>

# Kardeş Projeler

- [**Panda3DS**](https://github.com/wheremyfoodat/Panda3DS): Yazarlarımızdan wheremyfoodat tarafından geliştirilmiş çoklu platformlu bir 3DS emülatörü.
- [**hydra**](https://github.com/hydra-emu/hydra): Paris'ten chip-8, GB, NES ve N64 gibi sistemleri destekleyen çoklu platformlu bir emülatör.

# Lisans

- [**GPL-2.0 license**](https://github.com/shadps4-emu/shadPS4/blob/main/LICENSE)