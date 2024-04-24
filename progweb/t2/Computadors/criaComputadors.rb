require 'rubygems'
require 'active_record'

ActiveRecord::Base.establish_connection :adapter => "sqlite3",
                                        :database => "Tabelas.sqlite3"

ActiveRecord::Base.connection.create_table :computadors do |t|
    t.string :numSerie, limit: 4
    t.string :marca
    t.references :pessoa, foreign_key: true
end