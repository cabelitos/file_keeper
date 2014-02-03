package com.filekeeper.view;
import java.awt.EventQueue;

import javax.swing.JFrame;
import javax.swing.JPanel;
import java.awt.BorderLayout;

import javax.swing.JOptionPane;
import javax.swing.JPopupMenu;
import javax.swing.JScrollPane;
import javax.swing.JToolBar;
import javax.swing.JSeparator;
import javax.swing.JMenuBar;
import javax.swing.JList;
import javax.swing.JTree;
import javax.swing.JTable;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import com.jgoodies.forms.layout.FormLayout;
import com.jgoodies.forms.layout.ColumnSpec;
import com.jgoodies.forms.factories.FormFactory;
import com.jgoodies.forms.layout.RowSpec;
import net.miginfocom.swing.MigLayout;
import javax.swing.BoxLayout;
import javax.swing.JMenu;
import javax.swing.JMenuItem;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.SystemColor;
import javax.swing.border.SoftBevelBorder;
import javax.swing.border.BevelBorder;
import javax.swing.ListSelectionModel;

import org.eclipse.swt.events.DisposeEvent;

import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;


public class frmprincipal {

	private JFrame frmFileKeeper;
	private JTable table;

	/**
	 * Launch the application.
	 */
	public static void main(String[] args) {
	
		EventQueue.invokeLater(new Runnable() {
			public void run() {
				try {
					
					try {
			            // Set cross-platform Java L&F (also called "Metal")
			        UIManager.setLookAndFeel(
			            UIManager.getSystemLookAndFeelClassName());
				    } 
				    catch (UnsupportedLookAndFeelException e) {
				       // handle exception
				    }
				    catch (ClassNotFoundException e) {
				       // handle exception
				    }
				    catch (InstantiationException e) {
				       // handle exception
				    }
				    catch (IllegalAccessException e) {
				       // handle exception
				    }
					
					frmprincipal window = new frmprincipal();
					window.frmFileKeeper.setVisible(true);
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		});
	}

	/**
	 * Create the application.
	 */
	public frmprincipal() {
		initialize();
	}

	/**
	 * Initialize the contents of the frame.
	 */
	private void initialize() {
		frmFileKeeper = new JFrame();
		frmFileKeeper.setTitle("File Keeper");
		frmFileKeeper.setBounds(100, 100, 577, 441);
		frmFileKeeper.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		
		JMenuBar menuBar = new JMenuBar();
		frmFileKeeper.setJMenuBar(menuBar);
		
		JMenu mnArquivo = new JMenu("Arquivo");
		menuBar.add(mnArquivo);
		
		JMenuItem mntmSair = new JMenuItem("Sair");
		mntmSair.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				frmFileKeeper.dispose();
			}
		});
		
		mnArquivo.add(mntmSair);
		
		JMenu mnAes = new JMenu("Ação");
		menuBar.add(mnAes);
		
		JMenu mnSobre = new JMenu("Sobre");
		menuBar.add(mnSobre);
		frmFileKeeper.getContentPane().setLayout(null);
		
		JTree tree = new JTree();
		tree.setBounds(10, 0, 199, 382);
		frmFileKeeper.getContentPane().add(tree);
		
		table = new JTable();
		table.setFillsViewportHeight(true);
		table.addMouseListener(new MouseAdapter() {
			@Override
		    public void mouseClicked(MouseEvent me) {  
		        // Verificando se o botão direito foi pressionado  
		        if ((me.getModifiers() & MouseEvent.BUTTON3_MASK) != 0) {  
		            JPopupMenu menu = new JPopupMenu();  
		            JMenuItem cliqueme = new JMenuItem("Reverter para esta versão");  
		  
		            cliqueme.addActionListener(new ActionListener() {  
		                public void actionPerformed(ActionEvent ae) {  
		                    JOptionPane.showMessageDialog(null, "Fui clicado !");  
		                }  
		            });  
		  
		            menu.add(cliqueme);  
		            menu.show(table, me.getX(), me.getY());
		        }  
		    } 
		});
		table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
		table.setBorder(new SoftBevelBorder(BevelBorder.LOWERED, null, null, null, null));
		table.setBackground(SystemColor.control);
		table.setBounds(220, 0, 331, 382);
		table.setModel(new DefaultTableModel(
			new Object[][] {
				{"C:\\teste.txt", "31/01/2014"},
			},
			new String[] {
				"Caminho", "Data Modificação"
			}
		) {
			Class[] columnTypes = new Class[] {
				String.class, String.class
			};
			public Class getColumnClass(int columnIndex) {
				return columnTypes[columnIndex];
			}
		});
		table.getColumnModel().getColumn(0).setPreferredWidth(200);
		table.getColumnModel().getColumn(1).setPreferredWidth(92);
		
		//
		frmFileKeeper.getContentPane().add(table);
		
	}
}
