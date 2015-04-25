

# Agilia #

http://packages.agilialinux.ru/search.php?lname=vacuum-im

## Установка ##
```
mpkg-install vacuum-im
```

# ALT Linux #

http://prometheus.altlinux.org/en/Sisyphus/srpms/vacuum-im

# Arch Linux #

## Подключение ##

<b>_Не добавляйте оба репозитория одновременно!_<br>
Do not add both repositories at the same time!</b>

Добавить строки в конец файла <b>/etc/pacman.conf</b><br>
Add lines to the end of file <b>/etc/pacman.conf</b>

<h3>Vacuum-IM (release)</h3>

<pre><code>[network_messaging_vacuum-im_release_Arch_Extra]<br>
SigLevel = Never<br>
Server = http://download.opensuse.org/repositories/network:/messaging:/vacuum-im:/release/Arch_Extra/$arch<br>
</code></pre>

<h3>Vacuum-IM (unstable)</h3>

<pre><code>[network_messaging_vacuum-im_unstable_Arch_Extra]<br>
SigLevel = Never<br>
Server = http://download.opensuse.org/repositories/network:/messaging:/vacuum-im:/unstable/Arch_Extra/$arch<br>
</code></pre>

<h2>Установка</h2>
<pre><code>pacman -Syu<br>
pacman -S vacuum-im<br>
</code></pre>

<h1>Debian</h1>
<a href='http://www.notesalexp.org/'>http://www.notesalexp.org/</a>

<h1>Fedora</h1>

<a href='http://koji.russianfedora.ru/koji/packageinfo?packageID=100'>http://koji.russianfedora.ru/koji/packageinfo?packageID=100</a>

<h2>Подключение</h2>
<i>Пользователям russianfedora делать не нужно</i>
<pre><code>su -c 'yum localinstall --nogpgcheck http://mirror.yandex.ru/fedora/russianfedora/russianfedora/free/fedora/russianfedora-free-release-stable.noarch.rpm'<br>
</code></pre>

<h2>Установка</h2>
<pre><code>yum install vacuum<br>
</code></pre>

<h1>FreeBSD</h1>

<a href='http://www.freshports.org/net-im/vacuum-im'>http://www.freshports.org/net-im/vacuum-im</a>

<h1>Gentoo</h1>

Доступно в portage:<br>
<a href='http://packages.gentoo.org/package/net-im/vacuum'>http://packages.gentoo.org/package/net-im/vacuum</a>

Дополнительные плагины есть в оверлее <b>rion</b>:<br>
<a href='https://code.google.com/p/rion-overlay/source/browse/net-im/'>https://code.google.com/p/rion-overlay/source/browse/net-im/</a>

<h1>Mandriva</h1>

<a href='https://code.google.com/p/vacuum-im/downloads/list?q=Package:Mandriva'>https://code.google.com/p/vacuum-im/downloads/list?q=Package:Mandriva</a>

<h1>openSUSE</h1>

<a href='https://build.opensuse.org/project/show?project=home%3AEGDFree%3Avacuum-im%3Agit'>https://build.opensuse.org/project/show?project=home%3AEGDFree%3Avacuum-im%3Agit</a>

<h2>Подключение</h2>

Замените версию дистрибутива на нужную (доступны: <b>openSUSE_12.1</b>, <b>openSUSE_12.2</b>,  <b>openSUSE_12.3</b>, <b>openSUSE_Factory</b>) и выполните:<br>
<br>
<h3>vacuum-im (stable)</h3>
Для получения текущей стабильной версии:<br>
<pre><code>sudo zypper ar -f  http://download.opensuse.org/repositories/network/openSUSE_12.3/ vacuum-im<br>
</code></pre>

Если используется Tumbleweed:<br>
<pre><code>sudo zypper ar -f http://download.opensuse.org/repositories/openSUSE:/Tumbleweed/standard/ vacuum-im<br>
</code></pre>

<h3>vacuum-im (unstable, vcs)</h3>
Для получения новейшей версии из git-репозитория разработки:<br>
<pre><code>sudo zypper ar -f  http://download.opensuse.org/repositories/home:/EGDFree:/vacuum-im:/git/openSUSE_12.3/ vacuum-im<br>
</code></pre>

<h2>Установка</h2>
<pre><code>sudo zypper in vacuum-im<br>
</code></pre>

<h1>Ubuntu</h1>

<h2>Vacuum-IM (stable)</h2>


<h3>Подключение</h3>
<pre><code>sudo add-apt-repository ppa:vacuum-im/release<br>
sudo apt-get update<br>
</code></pre>

<h2>Vacuum-IM (unstable)</h2>


<h3>Подключение</h3>
<pre><code>sudo add-apt-repository ppa:vacuum-im/unstable<br>
sudo apt-get update<br>
</code></pre>

<h2>Установка</h2>
<pre><code>sudo apt-get install vacuum<br>
</code></pre>

<h1>Прочие</h1>
<a href='http://jawiki.ru/Vacuum#Linux.2FUnix-like_.D0.9E.D0.A1'>http://jawiki.ru/Vacuum#Linux.2FUnix-like_.D0.9E.D0.A1</a>