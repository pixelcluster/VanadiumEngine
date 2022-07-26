/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <Engine.hpp>

extern void configureEngine(vanadium::EngineConfig& config);

extern void init(vanadium::Engine& engine);

extern bool onFrame(vanadium::Engine& engine);

extern void destroy(vanadium::Engine& engine);

int main() {
    vanadium::EngineConfig config;
    configureEngine(config);
    vanadium::Engine engine = vanadium::Engine(config);
    init(engine);
    while(onFrame(engine) && engine.tickFrame()) {}
    destroy(engine);
}
