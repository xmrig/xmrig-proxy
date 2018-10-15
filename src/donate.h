/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DONATE_H__
#define __DONATE_H__


/*
 * Dev donation.
 *
 * Percentage of your hashing power that you want to donate to the developer, can be 0 if you don't want to do that.
 *
 * If you plan on changing this setting to 0 please consider making a one off donation to my wallet:
 * XMR: 48edfHu7V9Z84YzzMa6fUueoELZ9ZRXq9VetWzYGzKt52XU5xvqgzYnDK9URnRoJMk1j8nLwEVsaSWJ4fhdUyZijBGUicoD
 * BTC: 1P7ujsXeX7GxQwHNnJsRMgAdNkFZmNVqJT
 *
 * How it works:
 * First pool connection (up to 256 workers) always without fee.
 * Other connections first randomly switch to dev pool in range from 50 to 150 minutes, to reduce dev pool peak load.
 * Stays on dev pool at least kDonateLevel minutes.
 * Choice next donation time, with overime compensation. In proxy no way to use precise donation time.
 * You can check actual donation via API.
 */
constexpr const int kDefaultDonateLevel = 0;
constexpr const int kMinimumDonateLevel = 0;
constexpr const uint64_t kFreeThreshold = 256;


#endif /* __DONATE_H__ */
