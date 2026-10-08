// Punto único de importación de Preact + htm (sin paso de compilación).
import { h, render, Fragment } from "./vendor/preact.mjs";
import { useState, useEffect, useRef, useMemo, useCallback, useLayoutEffect } from "./vendor/hooks.mjs";
import htm from "./vendor/htm.mjs";

export const html = htm.bind(h);
export { h, render, Fragment, useState, useEffect, useRef, useMemo, useCallback, useLayoutEffect };
