'use strict';
//20/06/24

function Menu(id, timeout = 0) {
	this.id = id;
	this.timeout = timeout;
	this.html = document.getElementById(id);
	this.draw = (x, y, ms = this.timeout) => {
		if (ms !== 0) { setTimeout(() => this.draw(x, y, null), ms); }
		else {
			this.html.style.top = y + 'px';
			this.html.style.left = x + 'px';
			this.html.style.visibility = 'visible';
			this.html.style.opacity = '1';
			this.checkFlags();
		}
	};
	this.flags = {};
	this.checkFlags = () => {
		for (let option in this.flags) {
			const entry = $('[id=' + option + ']');
			if (!this.flags[option]()) {
				entry.attr('style', 'color: darkgray;');
			} else {
				entry[0].style.removeProperty('color');
			}
		}
	}
	this.functions = {};
	this.onClick = (option) => {
		if (this.track(option)) {
			this.functions[option]();
		} else { alert(option + ' does not exist.'); }
	}
	this.track = (option) => {
		return this.functions.hasOwnProperty(option);
	}
}

const ctxMenu = (() => {
	const m = new Menu('ctxMenu')
	m.functions.ctxMenuDelete = () => $('#Del').click();
	m.flags.ctxMenuDelete = () => fb.isLocked != '1' && selection.count;
	m.functions.ctxMenuQueue = () => $('#QueueItems').click();
	m.flags.ctxMenuQueue = () => selection.count;
	m.functions.ctxMenuDequeue = () => $('#DequeueItems').click();
	m.flags.ctxMenuDequeue = () => selection.count;
	m.functions.ctxMenuUndo = () => $('#Undo').click();
	m.flags.ctxMenuUndo = () => fb.isUndoAvailable == '1';
	m.functions.ctxMenuRedo = () => $('#Redo').click();
	m.flags.ctxMenuRedo = () => fb.isRedoAvailable == '1';
	return m;
})();

const plmItemMenu = ((plsIdx) => {
	const m = new Menu('plmItemMenu')
	const htmlMenu = document.getElementById('plmItemMenu');
	htmlMenu.innerHTML = '';
	m.functions.plmItemMenuLoad = () => retrieveState('SwitchPlaylist', plsIdx);
	m.flags.plmItemMenuLoad = () => true;
	{
		const entry = document.createElement('a');
		entry.id = 'plmItemMenuLoad';
		entry.innerHTML = '<b>Load </b><span>Dbl. click</span>';
		htmlMenu.appendChild(entry);
	}
	{
		const entry = document.createElement('hr');
		htmlMenu.appendChild(entry);
	}
	for (const menuType in smp.playlistManagerEntries) {
		const id = menuType.toLowerCase().replace(/[ ()[\]]*/g,'');
		if (id) {
			const data = smp.playlistManagerEntries[menuType].length > plsIdx
			? smp.playlistManagerEntries[menuType][plsIdx].name
			: '';
			m.functions[id] = () => {
				data 
					? command('CmdLine', '/run_main:"File/Spider Monkey Panel/Script commands/' + data, void(0), true)
					: void(0);
			}
			m.flags[id] = () => true;
			const entry = document.createElement('a');
			entry.id = id;
			entry.innerHTML = '<b>' + menuType + '</b>';
			htmlMenu.appendChild(entry);
		}
	}
	if (document.addEventListener) {
		document.addEventListener('click', function (e) {
			if (e.button === 0) { // Left click only
				m.html.style.opacity = "0";
				const option = $(e.target).context.id;
				if (m.track(option)) { m.onClick(option); }
				setTimeout(() => m.html.style.visibility = "hidden", 501);
			}
		}, {once: true});
	} else { // ie
		const callback = function (e) {
			m.html.style.opacity = "0";
			const option = $(e.target).context.id;
			if (n.track(option)) { n.onClick(option); }
			setTimeout(() => m.html.style.visibility = "hidden", 501);
			m.detachEvent('onclick', callback);
		};
		m.attachEvent('onclick', callback);
	}
	return m;
});

const plsTabMenu = (() => {
	const m = new Menu('plsTabMenu', 300);
	m.functions.plsMenuRename = () => $('#RenamePlaylist').click();
	m.flags.plsMenuRename = () => {
		const ap = fb.playlistActive;
		return ap !== "-1" && fb.playlists.length && $('#tabs').tabs('option', 'active') === ap;
	}
	m.functions.plsMenuRemove = () => $('#RemovePlaylist').click();
	m.flags.plsMenuRemove = () => {
		const ap = fb.playlistActive;;
		return ap !== "-1" && fb.playlists.length && $('#tabs').tabs('option', 'active') === ap;
	}
	m.functions.plsMenuNew = () => $('#CreatePlaylist').click();
	return m;
})();

if (document.addEventListener) {
	document.addEventListener('contextmenu', function (e) {
		const posX = e.clientX;
		const posY = e.clientY;
		const atContext = $(e.target);
		const menus = [ctxMenu, plsTabMenu];
		let menu;
		switch (true) {
			case atContext.parents('[id=playlist]').attr('id') === 'playlist': menu = ctxMenu; break;
			case atContext[0].classList.contains('ui-tabs-nav') || atContext.parents('.ui-tabs-nav').length !== 0: menu = plsTabMenu; break;
			default: break;
		}
		if (menu) {
			menu.draw(posX, posY);
			menus.forEach((m) => {
				if (m !== menu) {
					m.html.style.visibility = "hidden";
				}
			});
		}
		e.preventDefault();
		return false;
	}, false);

	document.addEventListener('click', function (e) {
		if (e.button === 0) { // Left click only
			ctxMenu.html.style.opacity = "0";
			plsTabMenu.html.style.opacity = "0";
			const option = $(e.target).context.id;
			const menus = [ctxMenu, plsTabMenu];
			menus.forEach((menu) => {
				if (menu.track(option)) { menu.onClick(option); }
			});
			setTimeout(function () {
				menus.forEach((menu) => {
					menu.html.style.visibility = "hidden";
				});
			}, 501);
		}
	}, false);
} else { // ie
	document.attachEvent('oncontextmenu', function (e) {
		const posX = e.clientX;
		const posY = e.clientY;
		const atContext = $(e.target);
		const menus = [ctxMenu, plsTabMenu];
		let menu;
		switch (true) {
			case atContext.parents('[id=playlist]').attr('id') === 'playlist': menu = ctxMenu; break;
			case atContext[0].classList.contains('ui-tabs-nav') || atContext.parents('.ui-tabs-nav').length !== 0: menu = plsTabMenu; break;
			default: break;
		}
		if (menu) {
			menu.draw(posX, posY);
			menus.forEach((m) => {
				if (m !== menu) {
					m.html.style.visibility = "hidden";
				}
			});
		}
		e.preventDefault();
		return false;
	});
	document.attachEvent('onclick', function (e) {
		ctxMenu.html.style.opacity = "0";
		plsTabMenu.html.style.opacity = "0";
		const option = $(e.target).context.id;
		const menus = [ctxMenu, plsTabMenu];
		menus.forEach((menu) => {
			if (menu.track(option)) { menu.onClick(option); }
		});
		setTimeout(function () {
			menus.forEach((menu) => {
				menu.html.style.visibility = "hidden";
			});
		}, 501);
	});
}