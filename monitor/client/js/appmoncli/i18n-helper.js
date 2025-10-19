/**
 * i18nHelper: context-aware and parameterized translation helper
 * 
 * Usage:
 *   await i18nHelper.load('zh'); // or 'en'
 *   i18nHelper.t('SAVE_BUTTON');
 *   i18nHelper.t('SAVE', {}, 'menu'); // context-aware
 *   i18nHelper.t('WELCOME_USER', {username: 'Alice'});
 *   i18nHelper.t('APP_STATUS', {app: 'AppA', port: 8080});
 */

const i18nHelper = {
  translations: {},
  lang: 'en',

  /**
   * Load translation JSON file for the given language.
   * Supports context-aware keys: { "context|KEY": "translation" }
   */
  async load(lang) {
    this.lang = lang;
    try {
      const res = await fetch(`./locales/${lang.substr(0, 2)}.json`);
      if (!res.ok) throw new Error("Translation file not found");
      this.translations = await res.json();
    } catch (e) {
      console.warn("Falling back to English translations.");
      const res = await fetch(`./locales/en.json`);
      this.translations = await res.json();
    }
  },

  /**
   * Get translation by key, with optional parameters and context.
   * @param {string} key - The translation key.
   * @param {object} vars - Parameters for interpolation.
   * @param {string} context - Optional context string.
   * @returns {string}
   */
  t(key, vars = {}, context = null) {
    let lookupKey = context ? `${context}|${key}` : key;
    let str = this.translations[lookupKey] || this.translations[key] || key;
    // Parameter interpolation: {param}
    for (const k in vars) {
      str = str.replace(new RegExp(`{${k}}`, 'g'), vars[k]);
    }
    return str;
  }
};
globalThis.i18nHelper = i18nHelper; // Make it globally accessible