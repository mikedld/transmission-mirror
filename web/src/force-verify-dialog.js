/* @license This file Copyright © 2020-2023 Mnemosyne LLC.
   It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
   or any future license endorsed by Mnemosyne LLC.
   License text can be found in the licenses/ folder. */

import { createDialogContainer } from './utils.js';

export class ForceVerifyDialog extends EventTarget {
  constructor(options) {
    super();

    // options: remote, torrents, callback, context
    this.options = options;
    this.elements = ForceVerifyDialog._create(options);
    this.elements.dismiss.addEventListener('click', () => this._onDismiss());
    this.elements.confirm.addEventListener('click', () => this._onConfirm());
    document.body.append(this.elements.root);
    this.elements.dismiss.focus();
  }

  close() {
    if (!this.closed) {
      this.elements.root.remove();
      this.dispatchEvent(new Event('close'));
      for (const key of Object.keys(this)) {
        delete this[key];
      }
      this.closed = true;
    }
  }

  _onDismiss() {
    this.close();
  }

  _onConfirm() {
    const { remote, torrents, callback, context } = this.options;
    if (torrents.length > 0) {
      remote.verifyTorrents(
        torrents.map((t) => t.getId()),
        true,
        callback,
        context,
      );
    }

    this.close();
  }

  static _create(options) {
    const { heading, message } = ForceVerifyDialog._createMessage(options);

    const elements = createDialogContainer('remove-dialog');
    elements.heading.textContent = heading;
    elements.message.textContent = message;
    elements.confirm.textContent = 'Verify';
    return elements;
  }

  static _createMessage(options) {
    let heading = null;
    let message = null;
    const { torrents } = options;
    if (torrents.length === 1) {
      heading = `Force verify local data of ${torrents[0].getName()}?`;
      message = `This torrent's progress will be rechecked without any safety checks. Are you sure?`;
    } else {
      heading = `Force verify local data of ${torrents.length} transfers?`;
      message = `These torrents' progress will be rechecked without any safety checks. Are you sure?`;
    }
    return { heading, message };
  }
}
