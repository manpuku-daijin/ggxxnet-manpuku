#!/usr/bin/perl --
# !usr/local/bin/perl

# Permission Setting
$g_per_nodelist	= 0600;
$g_per_repdir	= 0701;
$g_per_replay	= 0600;
$g_per_lockdir	= 0701;

# dat file
$g_datfile = "nodelist.dat";
$g_repdir  = "replay";

# node list
$g_nodeList;

# remote info
$g_ownNode;

# rc4
#use Crypt::RC4;
#$g_rc4pass = "aaa";

# connect time
$g_removeTime_idle = 70;	# 70sec  (�ʏ�60�b�ŃN���C�A���g����ē������)
$g_removeTime_busy = 600;	# 600sec (busy�͍ăG���g������Ȃ��A600�b����Ώ\��)

# lock dir
$g_lockdir = "lock";

##########################################################
if ($ENV{'REQUEST_METHOD'} eq POST)
{
	print "content-type: application/octet-stream\n\n";
	
	read(STDIN, $in, $ENV{'CONTENT_LENGTH'});
	#($name, $value) = split(/=/, $in);
	#$value =~ tr/+/ /;
	#$value =~ s/%([0-9a-fA-F][0-9a-fA-F])/pack("C", hex($1))/eg;
	
	(my$info, my$data) = split(/data=/, $in);
	@args = split(/\|/, $info);
	my$cmd;
	my$name;
	my$size;
	my$port;
	my$param;
	my$win = 0;
	foreach (0..@args-1)
	{
		(my$op, my$value) = split(/=/, $args[$_]);
		if ($op eq "cmd")  { $cmd  = $value; }
		if ($op eq "name") { $name = $value; }
		if ($op eq "size") { $size = $value; }
		if ($op eq "port") { $port = $value; }
		if ($op eq "param"){ $param= $value; }
		if ($op eq "win")  { $win  = $value; }
	}
	
	if ($cmd eq "enter")
	{
		# ����
		&enter($port, $name, $param, $win);
	}
	elsif ($cmd eq "read")
	{
		# �ŐV�m�[�h���X�g�̗v��
		&read();
	}
	elsif ($cmd eq "leave")
	{
		# �ގ�
		&leave($port, $name);
	}
	elsif ($cmd eq "repup")
	{
		# Replay�A�b�v���[�h
		&repup($name, $data, $size);
	}
	elsif ($cmd eq "repdown")
	{
		# Replay�_�E�����[�h
		&repdown($name);
	}
	elsif ($cmd eq "replist")
	{
		# Replay���X�g�擾
		&replist();
	}
}
else
{
	my$char = $ENV{'QUERY_STRING'};
	
	my$count = &usercount();
	
	if (1)
	{
		# ���[�U�J�E���gText
		print "content-type: text/html\n\n";
		print "user count = ".$count;
	}
	else
	{
		my$num = 0;
		if ($char == 1) { $num = $count % 10; }
		elsif ($char == 1) { $num = $count / 10 % 10; }
		elsif ($char == 2) { $num = $count / 100 % 10; }

		# ���[�U�J�E���gImage
		print "Content-type: image/gif\n\n";
		open(IMG,"img/".$num.".gif");
		binmode(IMG);
		binmode(STDOUT);
		print <IMG>;
		close(IMG);
	}
}
exit;

#########################################
sub enter # ����
{
my$p_port = $_[0];
my$p_name = $_[1];
my$p_param= $_[2];
my$p_win  = $_[3];

	$g_ownNode = &getOwnNode($p_port);
	if ($g_ownNode == 0)
	{
		# print "port error.";
		exit;
	}
	# print "own node = $g_ownNode<br>";
	
	&lockdat();
	&readNodeList();
	
	my$node = $p_name.'@'.$g_ownNode;
	my$i = &findNode($node);
	if ($i != -1)
	{
		# ����΍X�V
		$g_nodeList[$i] = time().'$'.$node.'%'.$p_param.'#'.$p_win;
	}
	else
	{
		# ������Βǉ�
		push(@g_nodeList, time().'$'.$node.'%'.$p_param.'#'.$p_win);
	}
	
	&writeNodeList();
	&unlockdat();
	
	print "##head##".$g_ownNode."##foot##\r\n";
}

#########################################
sub read # �m�[�h���X�g���擾
{
	&readNodeList();
	
	print "##head##\r\n";
	
	foreach (0..@g_nodeList-1)
	{
		my$i = $_;
		my$dlm = index($g_nodeList[$i], '$');
		if ($dlm != -1)
		{
			my$node = substr($g_nodeList[$i], $dlm+1, 9999);
			print $node."\r\n";
			
			# �����m�[�h���Q�ȏ�o�͂��Ȃ�
			#while (1)
			#{
			#	my$j = &findNode($node);
			#	if ($j == -1) { last; }
			#	$g_nodeList[$j] = "";
			#}
		}
	}
	print "##foot##\r\n";
}

#########################################
sub leave # �ގ�
{
my$p_port = $_[0];
my$p_name = $_[1];

	$g_ownNode = &getOwnNode($p_port);
	if ($g_ownNode == 0)
	{
		# print "port error.";
		exit;
	}
	# print "own node = $g_ownNode<br>";
	
	# $g_ownNode = encrypt($g_ownNode);
	# $g_ownNode = decrypt($g_ownNode);
	
	&lockdat();
	&readNodeList();
	
	while (1)
	{
		my$i = &findNode($p_name.'@'.$g_ownNode);
		if ($i == -1) { last; }
		splice(@g_nodeList, $i, 1);
	}
	
	&writeNodeList();
	&unlockdat();
}

#########################################
sub usercount # ���[�U�J�E���g
{
	# �ǂݍ���
	if (-e $g_datfile)
	{
		open(IN, $g_datfile);
		my@list = <IN>;
		return @list;
	}
	else
	{
		return 0;
	}
}

#########################################
sub repup # Replay�A�b�v���[�h
{
my($p_fname) = $_[0];
my($p_data)  = $_[1];
my($p_size)  = $_[2];

	mkdir($g_repdir, $g_per_repdir);
	
	# rpy�쐬
	$p_fname = time."_".$p_fname;
	open(OUT, "> $g_repdir/$p_fname");
	chmod($g_per_replay, "$g_repdir/$p_fname"); #0644
	
	syswrite(OUT, $p_data, $p_size);
	
	close(OUT);
	
	print "$p_size";
}

#########################################
sub repdown # Replay�_�E�����[�h
{
my($p_fname) = $_[0];
	
	print "##HEAD##";

	# rpy�ǂݍ���
	open(IN, "$g_repdir/$p_fname");
	binmode(IN);
	binmode(STDOUT);
	print <IN>;
	close(IN);

	print "##FOOT##";
}

#########################################
sub replist # Replay���X�g�擾
{
	# rpy�f�B���N�g����̑S�t�@�C���ǂݍ���
	opendir(DIR, "$g_repdir");
	@list = grep(!/^\./, readdir(DIR));
	
	foreach (0..@list-1)
	{
		print "$list[$_]\n";
	}
	
	closedir(DIR);
}

#########################################
#sub encrypt # �Í���
#{
#my$p_str = $_[0];
#	
#	$p_str = RC4($g_rc4pass, $p_str);
#	$p_str = unpack("H*", $p_str);
#	
#	return $p_str;
#}

#########################################
#sub decrypt # ������
#{
#my$p_str = $_[0];
#
#	$p_str = pack("H*", $p_str);
#	$p_str = RC4($g_rc4pass, $p_str);
#	
#	return $p_str;
#}

#########################################
sub getOwnNode # ���m�[�h���擾
{
my$p_port = $_[0];

	# �|�[�g�̃`�F�b�N
	if ($p_port < 1000 || $p_port > 65535) { return 0; }
	return "$ENV{'REMOTE_ADDR'}:$p_port";
}

#########################################
sub readNodeList # dat����m�[�h���X�g��ǂݍ���
{
	# �ǂݍ���
	if (-e $g_datfile)
	{
		open(IN, $g_datfile);
		
		my@dat = <IN>;
		
		foreach $line (@dat)
		{
			chomp($line);
			
			my$dlm = index($line, '$');
			my$plm = index($line, '%');
			if ($dlm != -1 && $plm != -1)
			{
				# ��莞�Ԃō폜�����i�������邽�߂ɒ���I�ɍăG���g�����K�v�j
				# �A���Abusy�͍ē�����Ȃ����ߍ폜�ΏۊO�A
				my$time = substr($line, 0, $dlm);
				my$busy = substr($line, $plm+1, 1);
				my$removeTime = 0;
				if ($busy == 0) { $removeTime = $g_removeTime_idle; }
				else { $removeTime = $g_removeTime_busy; }
				
				if ($time + $removeTime > time())
				{
					push(@g_nodeList, $line);
				}
				# print $busy.$line."\r\n";
			}
		}
		close(IN);
	}
}

#########################################
sub writeNodeList # dat�Ƀm�[�h���X�g����������
{
	# ��������
	open(OUT, "> $g_datfile");
	chmod($g_per_nodelist, "$g_datfile");
	
	foreach (0..@g_nodeList-1)
	{
		print OUT "$g_nodeList[$_]\n";
	}
	
	close(OUT);
}

#########################################
sub lockdat
{
	# �r��
	foreach (0..3)
	{
		if (mkdir($g_lockdir, $g_per_lockdir)) { last; }
		sleep(1);
	}
}


#########################################
sub unlockdat
{
	# �r���I��
	rmdir($g_lockdir);
}

#########################################
sub findNode # �m�[�h�����X�g����T��
{
my$p_nodeStr = $_[0];

	foreach (0..@g_nodeList-1)
	{
		my$dlm1 = index($g_nodeList[$_], '$');
		my$dlm2 = index($g_nodeList[$_], '%');
		if ($dlm1 != -1 && $dlm2 != -1)
		{
			my$node = substr($g_nodeList[$_], $dlm1+1, $dlm2 - ($dlm1+1));
			# print "$node<br>";
			if ($node eq $p_nodeStr)
			{
				return $_;
			}
		}
	}
	return -1;
}

#########################################
sub drawNodeList # �m�[�h���X�g��\���i�f�o�b�O�p�j
{
	my$count = @g_nodeList;
	print "node count = $count<br>";
	
	foreach (0..@g_nodeList-1)
	{
		print "$_-$g_nodeList[$_]<br>";
	}
}
